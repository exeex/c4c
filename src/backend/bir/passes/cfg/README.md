# P03 CFG Canonicalization Pass Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: pass
Phase-ID: B3 / P03
Upstream: exact immutable B2 / P02 `ScalarsCanonical` revision plus matching `Cfg`
Downstream: exact immutable B3 `CfgCanonical` revision plus freshly recomputed `Cfg`
Owner-Path: `src/backend/bir/passes/cfg/README.md`
Last-Reconciled-Commit: `878e56a97`

## Purpose

P03 is the target-independent B3 function transformation. It consumes the
exact accepted P02 checkpoint and same-key pre-planning CFG result, normalizes
blocks, typed terminators and edge occurrences, then publishes one verified
B3 checkpoint or fails atomically. Terminators remain the sole stored
successor authority.

## Owns

- the closed `PassId::CfgCanonicalize` / B3 occurrence;
- registered deterministic unreachable-region removal, branch/switch folding,
  successor-slot normalization, block split/merge and required critical-edge
  preparation;
- exact `EdgeKey` replacement and explicit-`Phi` incoming repair caused by its
  own topology edits;
- the P03 local/cumulative postcondition and request for `CfgCanonical` after
  framework verification;
- mandatory fresh CFG recomputation from the resulting terminators.

## Does Not Own

- P02 scalar/comparison/select form, P04 SSA construction or arbitrary phi
  value invention;
- stored predecessors, edge tables, reachability, traversal or dense numbering;
- target/profile/layout, ABI/helper selection, pseudo lowering, constraint
  interpretation, preparation, allocation, MIR or emission;
- inline-asm payload rewriting, local exception ABI, memory/call semantics;
- revision/property/cache/stage publication, which the frameworks own.

## Inputs

P03 consumes one immutable B2 checkpoint and one `AnalysisId::Cfg` handle whose
complete key matches each planned function exactly. P02 postconditions and the
configured input verifier accept that same revision. Analysis facts are
immutable planning evidence, never mutation or publication authority.

### Exact input and dependency matrix

| Input axis | Exact required state | Optional/non-provable form | Failure / forbidden substitution |
|---|---|---|---|
| owning checkpoint | exact successful immutable B2 occurrence | a proven P02 no-op retains the B2 stamp/property | wrong/missing capability is `CfgPassWrongInput` |
| module/function key | exact `ModuleEpoch`, `ModuleRevision`, `FunctionId`, `FunctionRevision` and ordered function digest | declaration/empty function is a valid no-op member | stale/foreign/mixed wave is `CfgPassStaleInput` |
| cumulative properties | `RawVerified` + `TypesLegal` + `ScalarsCanonical` on one checkpoint | none | missing/self-asserted property is `CfgPassWrongInput` |
| P02 acceptance | complete P02 postcondition and configured input verifier on that revision | no earlier-owner form may remain | mismatch/leftover is `CfgPassP02ContractFailure` |
| CFG dependency | schema-1 `AnalysisId::Cfg` handle with exact module/function/options key | empty body yields a complete empty result | missing/stale/foreign handle is `CfgPassStaleAnalysis` |
| terminator graph | stable block/inst IDs, typed terminators, exact role/index successor occurrences, complete def-use | zero-successor exit is valid | malformed/foreign/implied edge is inherited verifier failure |
| phi and preserved state | explicit-`Phi` incoming keyed by exact `EdgeKey`; opaque asm bytes/constraints and non-CFG semantics exact | phi may be absent; parallel occurrences remain distinct | arbitrary incoming value or opaque change is `CfgPassPreservedFormChanged` |
| target/helper exclusion | no target/profile/layout/ABI/helper/preparation/allocation/MIR fact | none | selected later-domain route is `CfgPassForbiddenAuthority` |

P03 has no fallback graph, cached earlier-revision handle, label lookup,
fallthrough guess, renderer parser, stored predecessor map or legacy route table.

## Outputs

The pipeline publishes the complete P03 function wave as one immutable B3
checkpoint. It retains the epoch, derives exact module/function revisions and
an authoritative `MutationSummary`, records B3, retains `RawVerified`,
`TypesLegal` and `ScalarsCanonical`, and establishes `CfgCanonical` only after
all functions and the mandatory post-P03 CFG recomputation succeed.

### Exact output handoff matrix

| Output/product | Exact consumer | Required binding and acceptance | Failure / forbidden escape |
|---|---|---|---|
| immutable B3 checkpoint | post-P03 analyses and B4/P04 | exact epoch/module/function revisions, ordered digest and B3 occurrence stamp | no function prefix, editor, B2 alias or mixed wave substitutes |
| cumulative properties | B4 precondition and later B8 verifier | `RawVerified` + `TypesLegal` + `ScalarsCanonical` retained; framework establishes `CfgCanonical` after proof | P03 cannot mint a property/report/capability itself |
| canonical terminator graph | fresh CFG computation, dominance/publication and B4 | every stored edge is one typed terminator role/index slot; block/edge identities and phi coverage exact | no predecessor/edge side table, fallthrough guess or dense identity escapes |
| occurrence `MutationSummary` | analysis/pass frameworks | exact committed blocks, terminators, slots, phis, uses and order effects | declarations cannot override observed effects or preserve old CFG handle |
| freshly recomputed `Cfg` | dominance/publication then B4/P04 | computed from resulting B3 terminators under the exact B3 key after mutation | pre-planning B2 handle or patched cache cannot substitute |
| structured diagnostics | pipeline observation only | stable rule/entity/edge anchor and deterministic order | cannot authorize partial publication, fallback graph or B4 |

## Adjacent-Stage Contract

[P02 scalar](../scalar/README.md) supplies the exact `ScalarsCanonical`
checkpoint. [CFG analysis](../../analysis/cfg/README.md) supplies matching
pre-planning facts. P03 alone owns the registered topology rewrites below; core
owns terminators/IDs, the pass framework owns transactions/publication, and
the analysis framework owns result recomputation/invalidation.

After candidate verification, CFG is recomputed from the resulting terminators
and accepted under the exact B3 key. Only then may
[dominance](../../analysis/dominance/README.md),
[publication](../../analysis/publication/README.md) and
[B4/P04 SSA](../ssa/README.md) consume the B3 checkpoint. B4 requires that
same immutable revision, `CfgCanonical`, exact-current CFG and its matching
dependencies.

## Ordered Behavior

1. Validate the exact B2 checkpoint/properties/P02 postcondition and same-key
   pre-planning `Cfg` handle.
2. Inventory every block, terminator, successor occurrence, unreachable region,
   split/merge candidate and exact phi incoming in canonical stable-ID order.
   Assign exactly one disposition from the closed matrix.
3. Prove every registered precondition without names, text, layout adjacency,
   target facts or collapsed predecessor sets. Accumulate deterministic rejects
   before editing.
4. Fork one private transaction per function in the occurrence wave. Reserve
   deterministic IDs and build a closed edit plan containing old/new
   `EdgeKey`s, terminators, blocks, moved instructions, uses and phi repairs.
5. Apply through the CFG editor, repair exact def-use and phi occurrence
   coverage, compose origins and derive per-function plus aggregate
   `MutationSummary` from actual edits.
6. Run local and cumulative Raw+P01+P02+P03 postconditions and the configured
   verifier on every private result. Recompute CFG from every resulting
   terminator set; validate multiplicity, reachability and phi coverage.
7. Only if the complete wave and recomputation succeed, atomically publish B3,
   establish `CfgCanonical` and install exact-B3 analysis results. Otherwise
   discard every candidate and retain the B2 checkpoint/cache unchanged.

## Invariants

- Terminators and ordered typed successor slots are the only stored local edge
  truth. Block order never implies fallthrough and no predecessor table exists.
- `BlockId` identifies a block; `EdgeKey {source, role, index}` identifies one
  occurrence. Parallel edges remain distinct even with equal destinations.
- Split replaces one old occurrence with two explicit new occurrences; merge,
  fold and deletion remap/remove every affected incoming by exact key.
- Explicit-`Phi` incoming key multisets equal live incoming occurrence multisets.
  P03 transfers existing values only by registered structural rules and never
  invents SSA values.
- Inline-asm text/constraints remain opaque; typed asm-goto successor slots are
  preserved. `MayUnwind` contributes no invented local successor.
- Stable IDs, typed atomic RAUW, complete def-use and deterministic origin
  composition survive every edit. Names/positions/text are never identity.
- No target/profile/layout/helper/ABI/preparation/allocation fact is consumed
  or produced.

## Exhaustive P03 CFG-Form Disposition Matrix

| Applicable P02 input form | Lossless facts / precondition | Disposition | Exact P03 result or stable failure | Next owner / B4 visibility | Implementation |
|---|---|---|---|---|---|
| declaration or valid empty function | no entry/body and complete empty CFG result | Preserve | unchanged; no failure | B4 sees empty exact CFG | absent |
| already canonical reachable block and terminator | exact slot order, identities, def-use and phi coverage | Preserve | unchanged; no failure | B4 sees canonical occurrence graph | absent |
| noncanonical semantic block order | closed stable semantic order proves reorder safe | Normalize | reorder blocks without changing edge truth; else `CfgRulePreconditionMismatch` | B4 sees canonical storage order | absent |
| unreachable block or closed unreachable region | exact unreachable set and no retained external semantic obligation | Normalize | remove blocks/instructions/uses/edges/incomings; else `CfgUnreachableRemovalInvalid` | B4 sees reachable canonical body | absent |
| constant conditional branch | exact typed constant and poison/exception policy proves chosen slot | Normalize | replace with one branch and delete exact other occurrences/incomings; else `CfgSemanticProofFailed` | B4 sees one exact edge | absent |
| conditional branch with equal destinations and proven equivalent incoming semantics | distinct slot occurrences and one exact replacement value is proved | Normalize | fold to one branch and repair incoming; else `CfgSemanticProofFailed` | B4 sees one exact edge | absent |
| conditional branch with equal destinations but no equivalence proof | distinct slot occurrences may carry different phi semantics | Preserve | retain both parallel occurrences; no failure | B4 sees both exact edges | absent |
| canonical nonconstant conditional branch | canonical predicate and role order | Preserve | unchanged; no failure | B4 sees both role-distinct occurrences | absent |
| invertible conditional with noncanonical role orientation | registered predicate inversion preserves semantics | Normalize | invert predicate and swap exact role slots/incomings; else `CfgSemanticProofFailed` | B4 sees canonical orientation | absent |
| switch with noncanonical case-slot order | unique exact typed case values and default | Normalize | reorder slots and remap incoming keys atomically; else `CfgSwitchInvalid` | B4 sees canonical case order | absent |
| switch with constant selector | exact case/default selection is provable | Normalize | replace with branch and delete other occurrences/incomings; else `CfgSemanticProofFailed` | B4 sees one selected edge | absent |
| default-only switch | no case slots and exact default destination | Normalize | replace with ordinary branch; else `CfgSwitchInvalid` | B4 sees branch | absent |
| switch slots sharing one destination | case identity and parallel edge multiplicity remain semantic | Preserve | retain distinct slot occurrences; no failure | B4 sees exact multiplicity | absent |
| duplicate switch case value | same typed value maps ambiguously | Reject | `CfgDuplicateCase` | no B3 output | absent |
| indirect branch with exact noncanonical target order | complete typed target-slot set | Normalize | canonicalize target slots and incoming keys; else `CfgIndirectTargetInvalid` | B4 sees exact ordered targets | absent |
| canonical indirect branch | complete typed targets and exact occurrence keys | Preserve | unchanged; no failure | B4 sees exact targets | absent |
| indirect branch with incomplete/foreign target | target set not closed in function | Reject | `CfgIndirectTargetInvalid` | no B3 output | absent |
| typed asm-goto terminator | exact opaque payload plus typed successor roles/indices | Preserve | topology/payload unchanged; malformed slot is `CfgAsmGotoInvalid` | B4 sees exact asm-goto edges | absent |
| `MayUnwind` escape without local slot | escape effect is not local topology | Preserve | no local edge invented; no failure | later owners retain escape semantics | absent |
| local exception/unwind successor in v1 | no registered canonical local exception form | Reject | `CfgUnsupportedLocalUnwind` | no B3 output | absent |
| branch-only forwarding block eligible by closed rule | exact one-in/one-out occurrence and phi transfer proof | Normalize | thread/remove block and remap every occurrence/incoming; else `CfgForwardingInvalid` | B4 sees simplified graph | absent |
| eligible adjacent block merge | one eligible connecting occurrence, safe instruction composition | Normalize | merge lists/terminators and remap affected incomings; else `CfgMergeInvalid` | B4 sees merged block | absent |
| required registered block split | exact split point, live values and two valid replacement terminators | Normalize | create deterministic block/terminator IDs and repair edges/uses; else `CfgSplitInvalid` | B4 sees explicit blocks | absent |
| registered critical-edge preparation | exact source occurrence and destination incoming coverage | Normalize | split that occurrence into two fresh exact edges; else `CfgCriticalEdgeInvalid` | B4 sees required prepared occurrence | absent |
| critical/parallel edge without P03 registered need | later D5 may require edge-local placement | Preserve | retain exact occurrence; no failure and no promise of universal splitting | D5 remains later owner | absent |
| phi incoming affected by a P03 edge rewrite | exact old occurrence/value and registered replacement map | Normalize | transfer/delete incoming by exact keys in same transaction; else `CfgPhiRepairInvalid` | B4 sees exact incoming multiset | absent |
| stored predecessor/edge side table or fallthrough-only edge | competing graph authority has no canonical form | Reject | `CfgForbiddenGraphAuthority` | no B3 output | absent |
| unfamiliar valid CFG form with no declared owner | no closed P03/later disposition exists | Reject | `CfgUnknownForm` | no B3 output | absent |

`Preserve` means the valid form and all its semantic facts remain exact; it
does not mean P03 silently delegates an owned noncanonical form. A conditional
row whose proof is absent follows its stated preserve result, not a speculative
fold. Every reject aborts the complete occurrence.

## Verification and Publication

The exact private P03 output must satisfy:

- complete Raw, P01 `TypesLegal`, P02 `ScalarsCanonical` and P03 CFG
  postconditions on one revision;
- one valid terminator per body block and every local edge derivable from
  exactly one typed successor occurrence;
- exact incoming occurrence multiplicity and explicit-`Phi` key coverage,
  including parallel edges;
- canonical stable-ID order, complete def-use, registered split/merge/fold
  results and no competing graph side table;
- preserved non-CFG/inline-asm semantics and no target/later-domain fact;
- mandatory CFG recomputation from resulting terminators, whose complete facts
  agree with the candidate and are ready for exact-B3 dependencies;
- a second P03 occurrence proposes no mutation.

Only the pass framework publishes the complete B3 function wave and
establishes `CfgCanonical`; only the analysis manager installs the recomputed
exact-B3 `Cfg`. P03 cannot mint either product or a stage token.

## Failure and Diagnostics

Stable failures include `CfgPassWrongInput`, `CfgPassStaleInput`,
`CfgPassP02ContractFailure`, `CfgPassStaleAnalysis`,
`CfgPassForbiddenAuthority`, `CfgRulePreconditionMismatch`, the matrix failures,
`CfgNonconvergent`, `CfgPostRecomputeMismatch`,
`CfgPassPreservedFormChanged`, deterministic resource exhaustion, cancellation,
verifier rejection and transaction/occurrence publication failure.

Diagnostics carry exact checkpoint/analysis keys, stable rule and typed
block/terminator/edge anchor in deterministic order. Failure rolls back the
entire wave and publishes no block prefix, edge/incoming repair, revision,
property, recomputed analysis/cache entry, report or stage token. The B2
checkpoint and its cache remain unchanged.

## Analysis and Invalidation

Any committed block membership/order, terminator, successor occurrence, phi
incoming, instruction move or relevant use change invalidates pre-planning
`Cfg`, dominance, publication/value-flow, loops, liveness, path-sensitive
effects/provenance and all transitive dependents. Old handles remain bound to
B2 and stale after a revision increment; P03 cannot patch or retarget them.

The framework derives exact mutation effects and evicts overlapping results.
Regardless of whether topology changed, P03's completion gate requests CFG
computation from the resulting terminators; downstream dominance/publication
are then computed against that exact post-P03 handle. Failure publishes no
candidate analysis. A true no-op may retain the same revision, but the explicit
post-P03 recomputation/validation still establishes the B4 handoff rather than
an unchecked preservation claim.

## Target and ABI Rules

P03 has no target profile/layout, helper decision, ABI/preparation product,
constraint binding, register/stack home, frame or MIR input. It cannot infer
fallthrough from layout, select branch encodings, interpret asm constraints or
materialize exception ABI. All output remains target-independent and
unallocated.

## Implementation State

Implementation is absent. No checked-in `PassId::CfgCanonicalize`
implementation/registration, rewrite registry, transaction route, verifier
hook, post-P03 recomputation path, build target or runtime proof exists. The
matrix is normative design coverage, not implemented support.

## Proof Requirements

- verify metadata/core-first order, substantive input/output matrices and all
  closed form dispositions;
- test exact P02/CFG key matching, stable edge-role/index multiplicity, parallel
  edges, switch/indirect/asm-goto and explicit phi repair;
- prove deterministic IDs/order, atomic full-wave rollback, cumulative
  postconditions and idempotence;
- test precise invalidation/transitive eviction and reject any old/patched CFG
  handle after mutation;
- prove mandatory post-P03 CFG recomputation precedes dominance/publication/B4
  and B4 accepts only the exact B3 revision;
- reconcile implementation/build truth against checked-in storage.

## Open Questions

None for v1. Local exception successors or additional topology forms require a
separate registered schema/owner decision; they cannot be inferred here.

## Review Checklist

- [x] Exact P02 checkpoint plus matching pre-planning CFG required.
- [x] Every CFG form has one normalize/preserve/reject disposition.
- [x] Terminators/slots remain the sole stored edge authority.
- [x] Parallel multiplicity, stable IDs and phi occurrence repair are exact.
- [x] Atomic transaction, rollback and cumulative properties are explicit.
- [x] Invalidation rejects old handles and post-P03 CFG is recomputed.
- [x] Exact B4 acceptance and target/later-domain exclusion are explicit.
- [x] Implementation truth is absent.
