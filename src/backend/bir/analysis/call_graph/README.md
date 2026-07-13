# Revision-Bound Call Graph and Call-Semantics Analysis Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: analysis
Applies-To: available from verified Raw; B7 / P07 earliest phase-B consumer
Upstream: exact immutable verified Raw-or-later module, exact B6 for P07
Downstream: B7 / P07 intrinsic planning and later exact-revision call queries
Owner-Path: `src/backend/bir/analysis/call_graph/README.md`
Last-Reconciled-Commit: `c0e3cdc6e`

## Purpose

`CallGraph` is an immutable target-independent module analysis deriving
semantic call topology and visibility from typed declarations and bodies. It
may be computed from verified Raw, but P07 is its earliest phase-B mutation
consumer and requires the exact B6 result. It owns neither call transformation
nor calling-convention/helper realization.

## Owns

- closed `AnalysisId::CallGraph`, schema version 1, module-scope traits;
- complete module/body-digest/options key and checked handle;
- stable function/declaration/external nodes, exact call-site facts, direct,
  finite-set indirect and unknown-indirect relations;
- callers/callees, SCC/recursion, address-taken/visibility and declared semantic
  call-effect observations;
- semantic equality, failure, stale rejection, invalidation and checked
  installation under a new complete key.

## Does Not Own

- call/intrinsic nodes, declarations, signatures, bodies, symbols or stable IDs;
- call rewriting, helper eligibility/selection, ABI placement or stage tokens;
- a callee inferred from name, pointer, text, position, provenance guess or
  legacy call-chain route;
- target features/layout, preparation, constraints, allocation, MIR or emission;
- `InlineAsm` as a call edge or an editable secondary instruction graph.

## Inputs

One frozen immutable verified Raw-or-later module is required. For P07 it is
the exact B6 `AggregatesCanonical` wave. Typed function/declaration/signature/
symbol tables, stable call `InstId`s, callee operands, call bundles, intrinsic
semantic identities, address-taken uses and declared effects are observed.

### Exact descriptor and input-key matrix

| Input/key axis | Exact required value | Optional/non-provable form | Failure / forbidden substitution |
|---|---|---|---|
| descriptor | `AnalysisId::CallGraph`, schema 1, `Module`, `CanonicalSemantic` | none | unknown/duplicate ID/schema is `CallGraphRegistryInvalid` |
| accepted stage | immutable verified Raw or exact later canonical checkpoint | empty module is valid | unverified/mixed capability is `CallGraphWrongInput` |
| module key | exact `ModuleEpoch` and `ModuleRevision` | empty module still carries the key | stale/foreign module is `StaleAnalysis` |
| body key | ordered digest of every `(FunctionId, FunctionRevision)` observed | no definitions yields canonical empty digest | missing/stale/mixed digest is `StaleAnalysis` |
| dependency key | empty ordered `dependencies` set | none | any dependency is `CallGraphRegistryInvalid` |
| options key | canonical empty schema-bound `AnalysisOptionsFingerprint` | v1 has no semantic option | nonempty/unsupported option is `CallGraphOptionsInvalid` |
| call semantics | typed call/intrinsic opcode, stable call ID, callee operand, signature/bundle/effects and declarations | indirect unresolved callee is valid `Unknown`; no call is known empty | malformed type/signature/bundle is `CallGraphInputInvalid` |
| symbol/visibility inputs | stable function/symbol IDs, declaration/definition, linkage, visibility and address-taken uses | external declaration has explicit external node; absent body is `Absent` | name/text/legacy substitution is `CallGraphForbiddenInput` |
| target/preparation axes | target-layout key `None`; preparation digest empty | none | target/helper/ABI/allocation input is `CallGraphForbiddenInput` |

The complete key binds descriptor/schema/domain, exact epoch/module revision,
the ordered complete function-revision digest, empty dependencies/options, no
target-layout key and empty preparation digest. Module revision alone never
proves freshness when any body is observed.

## Outputs

One immutable result uses stable IDs and keeps parallel call sites distinct,
even when caller/callee pairs repeat.

### Exhaustive call-graph result-family matrix

| Result family | Exact stable facts | Optional/non-provable form | Consumer and invalidation |
|---|---|---|---|
| function/declaration nodes | `FunctionId`/`SymbolId`, signature, linkage/visibility and body status | declaration body is `Absent`; external class explicit | P07 registry/call checks; table/signature change invalidates |
| direct call site | caller `FunctionId`, call `InstId`, exact callee `FunctionId`/declaration and typed signature | none | P07 direct-call/intrinsic planning; callee/signature change invalidates |
| finite indirect call site | call `InstId` and exact stable ordered finite callee set with proof rule | no proof degrades to unknown-indirect, not partial set | P07 preservation/effect checks; operand/set/proof change invalidates |
| unknown indirect call site | call `InstId`, typed signature and `Unknown(IndirectCalleeUnproved)` | valid conservative result | P07 preserves call; operand/type change invalidates |
| external/unknown callee relation | call ID, declaration/external class and typed boundary | unresolved external target is explicit `Unknown(ExternalResolution)` | P07 fail/preserve row; declaration/visibility change invalidates |
| caller/callee incidence | exact ordered call-site IDs per stable node | no callers/callees is known empty | P07 module planning; any edge/site change invalidates |
| SCC/recursion | deterministic SCC IDs represented by ordered stable function members and recursion class | declarations/external nodes may be outside body SCCs | P07 bounded module planning; edge/body change invalidates |
| address-taken/visibility | stable function/symbol and exact use/visibility class | no address-taking use is known false | P07 preservation; use/linkage change invalidates |
| declared call effects | exact call/declaration IDs and semantic effect descriptor | undeclared/opaque effect is `Unknown(CallEffectUnproved)` | P07 with memory effects; effect/declaration change invalidates |
| inline-asm observation | asm `InstId` classified as non-call ordinary opaque node | no asm is known empty | P07 must preserve non-edge status; asm semantic-shape change invalidates |
| semantic equality | all nodes/sites/sets/SCCs/status reasons in stable order | no equality inferred across keys | framework preservation only; never mutation authority |

Direct facts are `Known`; absent bodies/calls are explicit `Absent`/empty; valid
unresolved call semantics are stable-reason `Unknown`. Malformed typed calls,
registry inconsistency or foreign IDs fail and never become unknown edges.

## Adjacent-Stage Contract

[P06 aggregate](../../passes/aggregate/README.md) supplies the exact B6 module
wave. The manager computes or checked-installs `CallGraph` under that complete
key. [P07 intrinsics](../../passes/intrinsics/README.md) is the earliest
phase-B mutation consumer and may use facts only when the handle matches its
input exactly. The analysis cannot select a helper, rewrite a call or publish
the B7 candidate.

## Ordered Behavior

1. Validate descriptor/domain, exact module/body digest, empty dependencies/
   options and target exclusion.
2. Freeze declarations/signatures/symbols and every observed function body.
3. Create stable nodes and inventory typed call sites in canonical order.
4. Emit direct, proven finite-set indirect or stable-reason unknown-indirect
   facts without names, target or provenance guesswork.
5. Build incidence, deterministic SCC/recursion, visibility/address-taken and
   declared-effect products; validate complete call-site coverage.
6. Publish atomically only while every key axis remains current.

## Invariants

- Call `InstId` identifies a site; function/symbol stable IDs identify nodes.
  Caller/callee pairs never collapse parallel sites.
- A finite indirect set requires a registered complete proof. Otherwise the
  whole site is unknown-indirect, never a guessed subset.
- Calls describe semantic parameter/result positions and effects, never ABI
  locations, selected helpers or target availability.
- `InlineAsm` is one opaque ordinary-value node and never a call edge.
- Dense SCC indices, pointers, names, text and legacy routes do not escape.

## Failure and Diagnostics

Stable failures are `CallGraphRegistryInvalid`, `CallGraphWrongInput`,
`StaleAnalysis`, `CallGraphOptionsInvalid`, `CallGraphInputInvalid`,
`CallGraphForbiddenInput`, `CallGraphNonconvergent`, deterministic resource
exhaustion and cancellation. Diagnostics carry the complete module/body key
and stable function/symbol/call/rule anchors in deterministic order.

Failure publishes no partial node/edge/SCC/handle/cache entry. Stale dereference
returns `StaleAnalysis`; an old handle never rebinds or implicitly recomputes.

## Analysis and Invalidation

Any callee operand, call/intrinsic descriptor/effect, function signature/body,
declaration/definition, symbol/linkage/visibility, address-taken use, asm
semantic edge, module revision or observed function revision change invalidates
the graph and transitive dependents. After revision increment old handles are
stale. Checked preservation installs a complete new-key result only when
registered validators prove every node/site/set/SCC/status semantically equal.

## Target and ABI Rules

`CallGraph` is `CanonicalSemantic`, with target key `None` and empty preparation
digest. It rejects target features/layout, ABI locations, helper routes,
constraints, homes, spills, frames, MIR and renderer data.

## Implementation State

Implementation is absent. No checked-in descriptor, graph/SCC algorithm,
result schema, cache/invalidation validator, build target or runtime proof for
`AnalysisId::CallGraph` exists.

## Proof Requirements

- prove exact module/body-digest/options key and stale rejection;
- cover direct/finite-indirect/unknown/external/absent/malformed call forms;
- prove stable IDs, parallel site identity, atomic failure and invalidation;
- prove exact-B6 handle and P07 earliest phase-B use;
- reconcile implementation/build truth.

## Open Questions

None for v1. New indirect-call proof families require schema registration.

## Review Checklist

- [x] Exact module plus complete observed-body digest key is defined.
- [x] Direct, indirect, unknown, optional and malformed forms are distinct.
- [x] Stable nodes/sites and parallel call-site identity are exact.
- [x] Stale rejection, invalidation and checked preservation are explicit.
- [x] P07 is the earliest phase-B mutation consumer.
- [x] Target/helper/ABI/preparation inputs are excluded.
- [x] Implementation truth is absent.
