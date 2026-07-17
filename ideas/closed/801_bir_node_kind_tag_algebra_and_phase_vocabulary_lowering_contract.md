# BIR NodeKind Tag Algebra and Phase Vocabulary Lowering Contract

Status: Closed (capability complete)
Type: Architecture contract prerequisite with bounded schema proof
Extends: `ideas/closed/746_bir_node_kind_centric_storage_pass_contract.md`
Required Consumer: `ideas/open/732_bir_stage_document_convergence_umbrella.md`
Sequence: complete this idea before revising or reactivating idea 732

## Completion Evidence

Idea 801 is capability-complete. The normative contract is published at
`docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md`; its six-axis
taxonomy, SSA/B4 boundary, single query authority, B-through-F vocabularies and
transitions, identity rules, verifier gates, and fail-closed policy satisfy the
acceptance contract below.

Commit `2f569b624` lands the bounded C++17 convergence proof: one validated
16-kind registry derives compile-time and runtime queries, explicit stage
admission, payload checks, and negative behavior without adding phase passes,
storage changes, speculative production kinds, or adjacent importer/allocation/
MIR work. Matching supervisor-owned backend logs pass 6/6 before and after with
zero new failures. Commit `1373c07ff` completes the final normative status and
evidence handoff.

Idea 732 was not edited, revised, or activated by this lifecycle. Its overview
and phase children must adopt this normative artifact only through a later
user-authorized lifecycle operation; they may refine phase-local sequencing but
must not redefine the accepted taxonomy, query meanings, transition semantics,
identity rules, or catch-all rejection policy.

## Goal

Define and publish one closed `NodeKind` tag algebra and one explicit
phase-vocabulary lowering contract for BIR. All BIR phases continue to use the
same flat `Node` record, graph, arena, and stable identity model. Enum kinds
gain inheritance-like multiple classification through hidden C++ traits/tag
composition rather than actual enum inheritance, TableGen, `.td` files, or a
generated schema DSL.

The accepted contract must be strong enough for idea 732 and its phase
children to treat the taxonomy, query authority, stage vocabularies, and
transition rules as normative input rather than rediscovering or redefining
them independently in every phase document.

## Why This Exists

Closed idea 746 established `NodeKind` as the schema and pass-dispatch
authority, but its landed representation is intentionally shallow:

- `NodeKindDescriptor` exposes flat fields rather than composable semantic
  tags;
- it has no closed SSA/non-SSA/value/effect tag algebra;
- current kinds use broad all-stage legality masks;
- it does not define exact admitted input/output vocabularies or tag changes
  for the B-to-C, C-to-D, D-to-E, and E-to-F boundaries.

Idea 732 can converge Markdown shape and adjacency, but it is not by itself a
strong enough owner for this cross-phase semantic foundation. Without a prior
normative algebra, each phase can invent incompatible meanings for SSA,
effects, pseudo status, machine realizability, operand roles, or lowering
legality while still appearing locally documented.

This idea extends 746 without reopening or weakening its accepted result. It
turns the existing descriptor seed into a durable multiple-classification and
vocabulary-transition contract that later documentation and implementation
must consume.

## In Scope

### 1. Closed tag taxonomy

Define one finite, reviewed taxonomy grouped into these axes:

1. **Value model** — no ordinary result, ordinary single value, multiple or
   projected result policy, `SsaEligible`, explicitly non-SSA, and any
   phase-specific SSA vocabulary distinction required by the evidence.
2. **Semantic family** — arithmetic, compare, conversion, memory, aggregate,
   call, control/terminator, phi/merge, authority, intrinsic, preparation,
   pseudo, allocation action, and machine family as applicable.
3. **Effects and control** — pure, reads memory, writes memory, may trap,
   call-like, terminator, branch shape, and other stable kind-level effects;
   payload-dependent refinements must remain subordinate to kind dispatch.
4. **Stage vocabulary** — Raw, Canonical, Prepared, Pseudo/Preallocation,
   Allocated, and MIR-ready/Machine admission, with exact definitions rather
   than an all-stage default.
5. **Operand/result/type policy** — fixed or variable arity, operand-role
   schema, result-count policy, concrete-type source/constraint policy, and
   payload requirement/shape.
6. **MIR realizability** — one-record realizable, requires expansion,
   requires allocation/frame facts, machine-only, or forbidden at the MIR
   boundary.

The taxonomy must be closed enough that adding a new tag category or a new
kind-to-tag combination requires an explicit schema review. It must define
mutually exclusive groups, composable groups, and invalid combinations so a
free-form bag of booleans cannot emerge.

### 2. SSA terminology and validity boundary

Separate static kind classification from dynamic graph validity:

- `SsaEligible` or the accepted equivalent means that a kind participates in
  ordinary SSA when admitted to an SSA-governed published vocabulary.
- Static classification does not prove dominance, one-definition, phi-edge,
  use-def, or publication invariants for a graph instance.
- B4 owns establishment and verification of graph-stage SSA validity.
- A kind shared by Raw and Canonical vocabularies must not be described as
  timelessly SSA unless the design proves that statement. If its meaning
  changes at B4, the contract must use stage-qualified queries or split Raw
  and Canonical kinds so the static answer remains exact.

The durable API and documentation must not overload `is_op_ssa` to mean both
"kind belongs to the ordinary SSA family" and "this node in this graph is
currently valid SSA."

### 3. One query authority

Define compile-time tag queries and runtime wrappers derived from the same
traits/schema authority. Conceptually:

```cpp
template<NodeKind Kind, class Tag>
inline constexpr bool node_has_tag_v = /* one schema authority */;

template<NodeKind Kind>
inline constexpr bool is_ssa_eligible_v =
    node_has_tag_v<Kind, SsaEligibleTag>;

bool node_has_tag(NodeKind kind, NodeTag tag) noexcept;
bool is_ssa_eligible(NodeKind kind) noexcept;
```

Typelists, specializations, descriptor generation, and validation machinery
remain private to the schema layer. Pass authors receive a small stable helper
surface and must not inspect typelist internals or maintain parallel switch
tables.

### 4. Phase vocabularies and transition matrix

Define explicit admitted input and output `NodeKind`/tag vocabulary sets for
phases B, C, D, E, and F. Publish a normative transition matrix for:

```text
B -> C -> D -> E -> F
```

Every boundary row must name:

- admitted input kinds or closed kind groups;
- produced output kinds or closed kind groups;
- tags retained, added, and removed;
- kinds that remain unchanged and why that is semantically exact;
- kinds that must lower, expand, project, split, merge, or disappear;
- analysis/product prerequisites and the publication verifier;
- failure for unknown, premature, stale, or unhandled vocabulary.

No boundary may use an implicit catch-all pass-through. An input kind omitted
from the accepted set is rejected; an accepted kind without an explicit
transition rule is a pass/verifier failure.

### 5. Lowering and identity rules

Specify lowering over the same shared `Node`/graph/arena model, including:

- when a node may preserve `NodeId` because its operation identity and result
  identity remain exact while its kind/payload is rewritten;
- when replacement with a new node is required because semantic identity,
  result count/type, operand roles, effects, or stage ownership changes;
- when projection/extract/result nodes are required for multi-output forms;
- how uses, provenance, revision keys, analysis invalidation, and failure
  atomicity behave under replacement, insertion, deletion, expansion, merge,
  or projection;
- why arena slot stability does not by itself justify preserving semantic
  identity.

The contract must not require a new phase-specific `Node` class or storage
container.

### 6. Verifier and publication obligations

For each publication stage, state the required vocabulary, tag-combination,
payload, arity, operand/result/type, effect/control, identity, and dynamic
graph invariants. Verifiers must reject:

- unknown enum values;
- known kinds illegal at the current stage;
- invalid tag combinations or descriptor/schema divergence;
- unlowered kinds at a boundary that requires their elimination;
- catch-all-preserved nodes with no explicit transition rule;
- a claimed SSA publication that has only static `SsaEligible`
  classification but lacks B4 graph proof.

### 7. Durable normative artifact

Publish a durable Markdown contract, with the schema definition or generated
constexpr view where appropriate, that records:

- the closed taxonomy and invalid combinations;
- static versus graph-stage SSA semantics;
- the B-through-F admitted vocabularies and transition matrix;
- identity-preservation and replacement rules;
- publication/verifier obligations;
- the stable public query API and its single authority.

Idea 732 and each phase child must explicitly cite and consume this artifact.
They may refine phase-local details but must not redefine the tag taxonomy,
static-query meanings, or cross-phase transition semantics.

### 8. Bounded implementation proof

Make only the minimum C++ changes needed to prove that one hidden tag/traits
authority can derive both compile-time queries and runtime wrappers across a
representative set containing semantic, prepared, pseudo, and machine kinds.
The proof should also demonstrate schema validation for invalid combinations
and fail-closed unknown-kind/stage behavior.

This proof is not authority to enumerate every future kind or implement the
B-through-F lowering pipelines.

## Out of Scope

- Completing or changing idea 734's LIR-to-new-BIR importer/container route.
- Implementing any complete B, C, D, E, or F pass or the whole phase
  vocabulary.
- Rewriting the shared `Node`, graph, arena, operand storage, or stable-ID
  model.
- Resolving the general multi-result/result-vector normalization design beyond
  recording the lowering and projection policy needed by this contract.
- Implementing register allocation, spill/frame lowering, MIR construction,
  target emission, or machine encoding.
- Adding TableGen, `.td` files, code generation, reflection-driven schema
  generation, or another external/internal DSL.
- Reopening, editing, or re-closing idea 746.
- Revising, activating, or executing idea 732 in the same lifecycle slice.
- Treating the current stale post-734 parking language in idea 732 as already
  resolved. A later user-authorized lifecycle revision must update 732's
  sequencing and make this accepted artifact its normative prerequisite.

## Required Work Order

1. Audit the exact descriptor and stage-legality limits left by closed 746.
2. Define and review the closed tag axes, exclusivity/composition rules, and
   static-versus-dynamic SSA terminology.
3. Define the shared compile-time/runtime query authority and bounded proof
   kinds.
4. Define admitted B/C/D/E/F vocabularies and the complete transition matrix.
5. Define node identity preservation, replacement/projection, invalidation,
   failure atomicity, and verifier/publication rules.
6. Publish the normative artifact and, only if necessary, land the bounded C++
   schema/query proof.
7. Stop. In a later user-approved lifecycle operation, revise 732's stale
   parking/sequence language, require its children to consume this contract,
   and only then reactivate 732.

## Acceptance Criteria

- One durable artifact defines all six required taxonomy axes, the closed
  groups, composable groups, and invalid tag combinations.
- Static `SsaEligible`/ordinary-SSA participation is unambiguously distinct
  from B4-established graph-stage SSA validity; shared Raw/Canonical kinds are
  either stage-qualified or split when a timeless static answer is false.
- Compile-time queries and runtime wrappers demonstrably derive from one
  authority, while typelist/schema machinery remains hidden from pass code.
- B, C, D, E, and F each have explicit admitted input/output vocabulary sets,
  and the B-to-C, C-to-D, D-to-E, and E-to-F matrix names retained, added, and
  removed tags plus exact fail-closed behavior.
- The contract states reviewable identity-preservation versus
  replacement/projection rules for every lowering shape it admits over the
  shared `Node`/arena model.
- Every publication stage has explicit verifier obligations, including
  unknown-kind and unhandled-transition rejection with no catch-all
  pass-through.
- The limitations left by closed 746—flat descriptor fields, missing
  SSA/non-SSA tag algebra, and all-stage legality masks—are explicitly
  resolved rather than renamed.
- Idea 732 and its phase children have a named normative artifact ready to
  consume before their later lifecycle revision/reactivation.
- If C++ proof is needed, focused compile/build and schema tests cover
  representative semantic, prepared, pseudo, and machine kinds plus invalid
  combinations and unknown values. The proof remains bounded and does not
  claim phase-pass completeness.
- No `Node` storage rewrite, complete phase lowering, allocation/MIR
  implementation, 734 importer work, result-vector normalization, or 746
  reopening is included.

## Reviewer Reject Signals

- Reject a free-form collection of booleans or tags with no closed axes,
  mutual-exclusion rules, invalid combinations, or schema validation.
- Reject renaming existing flat `NodeKindDescriptor` fields or wrapping the
  current all-stage masks while claiming the missing tag algebra and phase
  transition contract are complete.
- Reject any `is_op_ssa`-style helper that treats eligibility as proof of
  dominance, one-definition, phi-edge, use-def, or B4 publication validity.
- Reject marking one Raw/Canonical shared kind timelessly SSA merely to avoid
  stage-qualified semantics or a justified vocabulary split.
- Reject separate compile-time traits, runtime tables, verifier switches, or
  per-pass classification lists that can drift from one another.
- Reject exposing typelists/template plumbing as the ordinary pass API or
  introducing TableGen, `.td`, generated tables, or another schema DSL.
- Reject a transition matrix that lists only phase names, broad families, or
  prose arrows without exact admitted sets, retained/added/removed tags,
  identity consequences, and failure behavior.
- Reject default/catch-all pass-through, unknown-kind tolerance, or preserving
  an unhandled kind because its underlying `Node` storage is shared.
- Reject preserving `NodeId` solely because an arena slot can be mutated when
  operation/result identity, type, result count, operand roles, or effects
  actually changed; likewise reject unconditional replacement when exact
  identity preservation is required and provable.
- Reject testcase-shaped kinds, named-case matchers, rendered-text probes, or
  representative proof cases encoded into production classification logic.
- Reject expectation downgrades, supported-to-unsupported changes, verifier
  weakening, or test-only rewrites offered as schema capability progress.
- Reject helper renames, documentation headings, classification-only edits,
  or a small green test set claimed as complete B-through-F lowering.
- Reject broad `Node`/arena/storage, importer, allocation, MIR, emission,
  result-normalization, or whole-pass rewrites outside this prerequisite.
- Reject retaining 746's exact failure mode—flat duplicated facts and
  all-stage legality—behind new tag names or runtime wrappers.
- Reject editing/reopening closed 746, silently revising/activating 732, or
  absorbing 734 work into this idea.

## Lifecycle Handoff

The user approved and activated this contract as the sole active plan. After
this prerequisite is accepted and closed, a separate user-authorized lifecycle
revision should update idea 732's stale post-734 parking language, cite the
published tag/vocabulary artifact as normative input, and then decide whether
to reactivate 732.
