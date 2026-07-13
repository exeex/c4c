# P02 Scalar Canonicalization Pass Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: pass
Phase-ID: B2 / P02
Upstream: exact immutable B1 `TypesLegal` revision plus matching `ComparisonSelect`
Downstream: one immutable `ScalarsCanonical` revision consumed by B3 / P03
Owner-Path: `src/backend/bir/passes/scalar/README.md`
Last-Reconciled-Commit: none

## Purpose

P02 is the second target-independent canonical pass. It consumes the exact P01
`TypesLegal` checkpoint and the matching immutable comparison/select analysis,
then normalizes portable scalar expressions, comparisons, casts and selects
without changing their semantics or selecting a target/helper/ABI route.

Success publishes one immutable B2 checkpoint satisfying cumulative
`RawVerified`, `TypesLegal` and `ScalarsCanonical` postconditions. B3 alone owns
the following CFG normalization.

## Owns

- the closed `PassId::ScalarCanonicalize` / B2 function-pass occurrence;
- deterministic commutative operand order and closed scalar expression form;
- portable constant/identity/algebraic rewrites with exact poison, undef,
  overflow, trapping, floating and exceptional proofs;
- canonical comparison predicate/orientation and exact comparison folding;
- redundant scalar-cast removal and closed cast-chain composition;
- canonical select condition/arm/result and bounded select-chain rewriting;
- closed portable expansion of registered wide/special semantic operations;
- the P02 cumulative postcondition and request for framework establishment of
  `ScalarsCanonical` after the complete occurrence succeeds.

Every applicable form has exactly one `Normalize`, `Preserve`, or `Reject`
row. Analysis facts justify only a registered P02 rule; they never create
mutation authority by themselves.

## Does Not Own

- P01 type/constant-payload/opcode legality, A2 repair, import or storage;
- P03 branches/blocks/successor slots, P04 phi/SSA, P05 memory/address/atomic,
  P06 aggregate or P07 intrinsic canonicalization;
- comparison-analysis computation/caching, core IDs/revisions/editors,
  framework transactions/invalidation or verifier/publication gates;
- target/profile/layout decisions, runtime-helper symbol/route selection, ABI
  classification, preparation, constraints, pseudo lowering, allocation,
  frame state, MIR, encoding, emission or assembly parsing;
- names, rendered expressions, legacy comparison/select routes, physical
  flags/register state or dense analysis indices as semantic evidence.

P02 cannot invoke/reorder another pass, fuse a branch, change CFG successor
topology, reinterpret opaque inline asm or retain a form without a named owner.

## Inputs

P02 consumes one immutable B1 checkpoint with the exact module/function
revisions and P01 occurrence stamp plus one `ComparisonSelect` handle whose
complete key matches the function being planned. Both are required. The
analysis may contain `Unknown`/`Absent`; those cause preservation when no
registered rule is provable, never guesswork or failure by themselves.

### Exact input and dependency matrix

| Input axis | Exact required state | Optional/non-provable form | Failure / forbidden substitution |
|---|---|---|---|
| owning checkpoint | exact immutable successful B1 occurrence | a proven P01 no-op still has the B1 stamp/property | wrong/missing capability is `ScalarWrongInput` |
| module/function key | exact `ModuleEpoch`, `ModuleRevision`, `FunctionId`, `FunctionRevision` and ordered function digest | declarations/empty bodies are valid no-op functions | stale/foreign/mixed revision is `ScalarStaleInput` |
| cumulative properties | `RawVerified` plus `TypesLegal` on the same checkpoint | none | missing or self-asserted property is `ScalarWrongInput` |
| P01 acceptance | complete P01 postcondition and configured input verifier for that revision | no P01-owned Raw form may remain | mismatch/leftover is `ScalarP01ContractFailure` |
| comparison dependency | schema-1 `AnalysisId::ComparisonSelect` handle with exact module/function/options key | `Known`, `Absent` and stable-reason `Unknown` facts are valid | missing/stale/foreign key is `ScalarStaleAnalysis` |
| typed semantic graph | stable IDs, legal types/opcodes, exact constants, def-use, terminator-owned CFG and source-semantic attributes | special operations may remain only with their P01-declared disposition | malformed graph is inherited verifier failure; text/legacy repair is forbidden |
| opaque/preserved families | exact inline-asm bytes/edges and later-owner CFG/SSA/memory/aggregate/intrinsic forms | unfamiliar valid analysis relation remains unchanged | changing one is `PreservedFormChanged` |
| target/helper exclusion | no target/profile/layout/ABI/helper/preparation/allocation/MIR input | helper-eligible semantic identity may remain opaque and typed | any selected route/fact is `ScalarForbiddenAuthority` |

P02 has no fallback analysis, cached earlier-revision handle, route table,
rendered predicate parser or environment-dependent feature input.

## Outputs

The pipeline publishes the complete P02 function wave as one immutable B2
checkpoint. It retains the input epoch, derives exact module/function revisions
and one authoritative `MutationSummary`, records B2 in the stage stamp, retains
`RawVerified`/`TypesLegal`, and establishes `ScalarsCanonical` only after every
function transaction and cumulative postcondition succeeds.

### Exact output handoff matrix

| Output/product | Exact consumer | Required binding and acceptance | Failure / forbidden escape |
|---|---|---|---|
| immutable B2 function/module checkpoint | [B3/P03 CFG](../cfg/README.md) | exact epoch/module/function revisions, ordered digest and B2 occurrence stamp | no function prefix, candidate editor, earlier checkpoint or mixed wave may substitute |
| cumulative properties | B3 precondition and later B8 cumulative verifier | `RawVerified` + `TypesLegal` retained; framework establishes `ScalarsCanonical` after P02 proof | P02 cannot mint a property/report/capability itself |
| canonical scalar graph | B3 and exact-revision consumers | unique scalar/comparison/cast/select forms; later-owner families and inline asm preserved | no helper/target fact, compatibility identity, side table or unregistered special form |
| occurrence `MutationSummary` | analysis manager/pass framework | exact committed changed functions/entities/effects | declared preservation cannot override observed effects; stale input analysis never crosses |
| structured failure diagnostics | pipeline observation only | stable rule/entity/source and deterministic order | cannot select a fallback result, publish analysis or authorize B3 |

B3's exact input clause requires this same immutable function view,
`ScalarsCanonical`, P02 postconditions and the configured verifier for the same
revision. Terminators remain the sole stored edge authority.

## Adjacent-Stage Contract

[P01](../legalize/README.md) supplies the exact `TypesLegal` checkpoint.
[Comparison/select analysis](../../analysis/comparison/README.md) supplies the
matching immutable planning facts through the analysis framework. P02 alone
owns the scalar rewrites below; the pass framework owns transactions,
revisions, occurrence publication and invalidation. [B3/P03](../cfg/README.md)
accepts only the exact successful P02 output.

An analysis `Unknown` or failed rule precondition preserves the valid input
form under its existing owner. It never permits a speculative rewrite. A
reject disposition aborts the complete occurrence before publication.

## Ordered Behavior

1. Validate the B1 capability/stamp/properties, exact function key, P01
   postcondition and same-key `ComparisonSelect` handle.
2. Inventory every scalar/comparison/cast/select/special candidate in canonical
   stable-ID order without mutation. Assign exactly one matrix disposition.
3. Resolve registered rule preconditions using typed facts and analysis
   `Known` facts only. Preserve valid `Unknown`/`Absent` cases; accumulate every
   deterministic reject before editing.
4. Fork one private transaction per function in the occurrence wave. Build a
   deterministic rewrite plan and reserve replacement IDs before mutation.
5. Apply exact replacements and typed atomic RAUW, repair complete def-use and
   compose origins. Derive each function summary and the aggregate authoritative
   `MutationSummary` from actual edits.
6. Run P02 local and cumulative Raw+P01+P02 postconditions plus the configured
   verifier-on-commit on every private result.
7. If every function succeeds, atomically publish the complete wave and let the
   framework establish `ScalarsCanonical`; otherwise roll back/discard every
   unpublished function candidate and retain the B1 checkpoint unchanged.

## Invariants

- Integer widths, explicit signedness-sensitive semantics, shift policy,
  overflow/trap behavior and exact constant bits remain unchanged.
- Floating format, rounding/exception behavior, NaN class/payload and signed
  zero remain exact; host floating evaluation is never semantic proof.
- `Undef`/`Poison` are never observed as concrete values. A fold preserves
  laziness, poison propagation and every exceptional path.
- Comparisons use the closed P01 predicate set and deterministic orientation;
  pointer relationships are never manufactured through integer casts.
- `Select` retains one `i1` condition and equal typed arms/result. Chain work is
  bounded, cycle-safe and idempotent.
- Shape-changing rewrites use deterministic IDs, typed atomic RAUW, complete
  def-use repair and composed origins. Names/positions/text never identify a
  value.
- CFG successor topology, phi edges, memory effects, aggregate topology,
  intrinsic identity, source-semantic attributes and opaque inline-asm payload
  remain unchanged unless a later owning pass acts.

## Exhaustive P02 Scalar-Form Disposition Matrix

`Normalize` requires one registered portable semantic proof. `Preserve` leaves
the exact valid form unchanged when it is already canonical, non-provable or
owned later. `Reject` is stable whole-occurrence failure. The final row closes
the table; there is no catch-all success.

| Portable input form/family | Exact facts that must remain lossless | Disposition | Exact P02 result and stable failure | Analysis requirement / next owner | Implementation truth |
|---|---|---|---|---|---|
| already canonical non-candidate scalar operation | opcode, types, operands/results and exceptional semantics | Preserve | unchanged typed node; change is `PreservedFormChanged` | direct typed inspection; B3 | absent |
| commutative operation with noncanonical stable operand order | exact operands/types/flags and semantic commutativity proof | Normalize | deterministic stable-ID operand order; failed proof is `ScalarSemanticProofFailed` | `Known` use/producer safety | absent |
| noncommutative or order-sensitive operation | exact operand order and exceptional behavior | Preserve | unchanged typed node; swap attempt is `RulePreconditionMismatch` | analysis may be `Unknown`; B3 | absent |
| registered integer constant-fold candidate | exact width/bits/signedness/overflow/poison policy | Normalize | exact typed constant or `ScalarSemanticProofFailed` | `Known` constant operands | absent |
| registered integer identity/algebraic candidate | exact operands, use identities and poison/trap behavior | Normalize | proven equivalent operand/result or `ScalarSemanticProofFailed` | `Known` producers/uses | absent |
| registered floating constant-fold candidate | format/bits/rounding/exceptions/NaN/signed zero | Normalize | exact-format constant or `ScalarSemanticProofFailed` | `Known` constants and closed rule | absent |
| valid floating candidate without full proof | complete original floating semantics | Preserve | unchanged operation; mutation attempt is `RulePreconditionMismatch` | `Unknown(EquivalenceUnproved)`; B3 | absent |
| comparison with noncanonical predicate/orientation | domain, ordered/unordered/signedness and operand identity | Normalize | closed deterministic predicate/orientation or `ScalarSemanticProofFailed` | `Known` compare descriptor/equivalence | absent |
| registered constant comparison | exact typed operands, predicate and poison/undef behavior | Normalize | canonical `i1` constant or `ScalarSemanticProofFailed` | `Known` constants/compare | absent |
| pointer comparison not admitted by a closed rewrite | pointer identities/address spaces and predicate | Preserve | unchanged comparison; integer-cast route is `ScalarForbiddenAuthority` | analysis relation may be `Unknown`; B3 | absent |
| redundant scalar cast | exact source/destination type and value semantics | Normalize | original value through typed RAUW or `ScalarSemanticProofFailed` | `Known` transparent producer/use safety | absent |
| composable scalar cast chain | every intermediate type, conversion and exceptional rule | Normalize | one closed equivalent cast or `ScalarSemanticProofFailed` | `Known` bounded chain | absent |
| valid non-composable cast | exact cast/value/type chain | Preserve | unchanged chain; composition attempt is `RulePreconditionMismatch` | `Unknown(ChainUnproved)`; B3 | absent |
| inverted select condition with swappable arms | exact `i1` condition, arms/result and inversion proof | Normalize | canonical condition plus swapped typed arms or `ScalarSemanticProofFailed` | `Known` inversion | absent |
| select with provably identical arms | condition, arm identity/type and poison/lazy semantics | Normalize | exact arm value through typed RAUW or `ScalarSemanticProofFailed` | `Known` equivalence/use safety | absent |
| select with proven constant condition | condition truth, chosen arm and lazy poison semantics | Normalize | selected typed arm or `ScalarSemanticProofFailed` | `Known` typed constant condition | absent |
| registered bounded select-chain candidate | root/member IDs, types, conditions, arms and use graph | Normalize | one canonical bounded chain or `ScalarNonconvergent` | `Known` nesting/equivalence | absent |
| valid non-provable select relation | exact condition/arms/result/nesting | Preserve | unchanged select; rewrite attempt is `RulePreconditionMismatch` | `Unknown`/`Absent`; B3 | absent |
| registered `WideIntegerOp` portable expansion | exact width, operands/results, flags and exceptional semantics | Normalize | complete canonical scalar expansion or `UnsupportedCanonicalSemantics`; partial expansion is forbidden | closed scalar rule; B3 | absent |
| other registered special portable expansion | exact semantic operation ID/types/results/effects | Normalize | complete registered expansion or `UnsupportedCanonicalSemantics` | closed rule; named resulting owners | absent |
| special semantic node with exact later owner | complete node identity/types/operands/results/effects | Preserve | unchanged registered node; change is `PreservedFormChanged` | named P06/P07 or later semantic owner | absent |
| helper-eligible semantic operation | semantic operation identity/types/effects only | Preserve | unchanged semantic node; helper choice is `ScalarForbiddenAuthority` | C8/later helper owner after Canonical | absent |
| `InlineAsm` ordinary semantic node | ordinary inputs/results, opaque bytes, clobbers/effects/topology | Preserve | byte/edge-identical node; change is `OpaquePayloadChanged` | P07 validates; C9 later binds constraints | absent |
| CFG/SSA/memory/aggregate/intrinsic later-owner form | exact typed owner payload, stable IDs/edges/effects | Preserve | unchanged payload; change is `PreservedFormChanged` | P03-P07 exact owner | absent |
| valid form with no exact portable canonical representation | complete typed form and stable entity anchor | Reject | `UnsupportedCanonicalSemantics`; publish nothing | none | absent |
| valid retained form with no declared later owner | complete typed form and stable entity anchor | Reject | `MissingDownstreamDisposition`; publish nothing | none | absent |
| unknown/unlisted applicable P02 form | descriptor/rule ID, types and stable entity anchor | Reject | `UnknownScalarForm`; publish nothing; no catch-all preserve | none | absent |

## Verification and Publication

The exact private P02 output must satisfy:

- the complete Raw registry and P01 `TypesLegal` postcondition still hold;
- every scalar/comparison/cast/select form has its unique closed P02 result;
- every registered rewrite proves exact poison/undef/exception/floating and
  use/identity semantics; no partial special expansion remains;
- every preserved later-owner family and opaque inline-asm payload is exact;
- stable IDs, deterministic order, complete def-use and terminator-only CFG
  authority remain coherent;
- no target/profile/layout/helper/ABI/preparation/allocation fact appears;
- a second P02 occurrence with the same semantic options proposes no mutation.

Only the pass framework may publish the complete function wave and establish
`ScalarsCanonical`. P02 cannot mint a property, analysis result, checkpoint or
stage token. B8 later reruns cumulative Raw+P01+P02 obligations on the exact
frozen P07 candidate.

## Failure and Diagnostics

Stable failures include `ScalarWrongInput`, `ScalarStaleInput`,
`ScalarP01ContractFailure`, `ScalarStaleAnalysis`, `ScalarForbiddenAuthority`,
`UnknownScalarForm`, `RulePreconditionMismatch`, `ScalarSemanticProofFailed`,
`ScalarNonconvergent`, `UnsupportedCanonicalSemantics`,
`MissingDownstreamDisposition`, `PreservedFormChanged`,
`OpaquePayloadChanged`, deterministic resource exhaustion, cancellation,
verifier rejection and transaction/occurrence publication failure.

Diagnostics carry the exact input/analysis keys, stable rule and typed entity
anchor in deterministic order. Failure rolls back the entire wave and
publishes no revision, property, partial fold/function prefix, analysis/cache
entry, reusable report or stage token. The prior B1 checkpoint and its cache
remain unchanged; it is not relabeled as B2 success.

## Analysis and Invalidation

`ComparisonSelect` is a required exact-revision planning input and never
mutation authority. Any P02 change to an observed opcode, predicate, operand,
result, cast, select, type, constant, use list, block/order or terminator
condition invalidates that result. The old handle remains keyed to B1 and is
stale after a revision change; P02 cannot patch or retarget it.

The framework derives each function and aggregate `MutationSummary`, then
invalidates every overlapping analysis and transitive dependent according to
registered traits. Comparison/publication flow and any effect/provenance/
liveness result observing changed uses are evicted unless a mutation-specific
validator plus semantic-equality audit installs a fresh immutable result under
the exact B2 key. An empty no-op summary may retain same-key handles. Failure
publishes no candidate analysis. B3 requests fresh exact-B2 CFG facts in Step 3.

## Target and ABI Rules

P02 has no target profile/layout, helper registry decision, ABI/preparation
product, constraint binding, register/stack home, frame or MIR input. It
preserves source-semantic typed facts but cannot decide target legality,
convert pointer comparisons through target integers, select helpers/opcodes,
split values into physical lanes or realize inline-asm constraints. C1/C2/C8/C9
and later phases retain those authorities.

## Implementation State

Implementation is absent. This directory contains only this README. No
`PassId::ScalarCanonicalize` implementation/registration, scalar rule registry,
transaction/postcondition, build edge, analysis integration or focused proof
is checked in. Pass-framework scaffolding and legacy comparison/select code do
not implement this owner.

## Proof Requirements

- require exact metadata/core-first/detail order and resolve every link;
- require one exact P01 checkpoint plus same-key required comparison handle;
- require every closed P02 form to name lossless facts, one disposition, exact
  result/stable failure, analysis requirement/next owner and implementation;
- prove typed semantic rules for poison/undef/overflow/traps/floating/pointers,
  not rendered or named-case matching;
- prove one atomic function wave, exact revisions/stamp, cumulative
  postconditions, rollback and framework-owned property publication;
- prove mutation-derived invalidation, stale-handle rejection and no cache
  publication on failure;
- prove exact B3 `ScalarsCanonical` acceptance and implementation absence;
- reject target/helper/ABI interpretation, catch-all preservation, unsupported
  downgrade, expectation weakening and testcase-shaped proof.

## Open Questions

No open P02 question authorizes a helper/target/ABI route, adjacent-owner edit
or implementation. A new portable form requires a closed semantic rule and
matrix disposition; otherwise `UnknownScalarForm` remains fail-closed.

## Review Checklist

- [x] Metadata spine and core-first/detail order are exact.
- [x] Input binds exact P01 and `ComparisonSelect` keys on one immutable revision.
- [x] Known/Absent/Unknown analysis forms cannot authorize speculation.
- [x] Every P02 form has one Normalize/Preserve/Reject disposition and stable result/failure.
- [x] Scalar/comparison/cast/select semantics preserve poison/undef/exception/floating rules.
- [x] Transaction, cumulative postconditions, rollback and property publication are exact.
- [x] Invalidation is mutation-derived and old handles remain stale.
- [x] B3 accepts only the exact immutable `ScalarsCanonical` output.
- [x] Target/helper/ABI authority is excluded and implementation is absent.
