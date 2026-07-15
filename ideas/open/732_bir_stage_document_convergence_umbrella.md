# BIR B-F Pass Documentation Convergence

Status: Open (active)
Type: Documentation-only architecture and pass-planning convergence

## Goal

Converge the BIR Markdown architecture from the factual Raw-BIR publication
boundary through phases B, C, D, E, and F. Pre-plan every ordered pass so a
later implementation agent can identify its exact input vocabulary, output
vocabulary, `NodeKind`/tag transition, identity policy, verifier/publication
gate, analysis dependencies, invalidation, and failure behavior without
re-deriving the architecture.

This idea consumes the landed shared-node and NodeKind contracts. It does not
redesign them. Its output is Markdown only.

## Authoritative Inputs

### 1. Shared BIR storage and identity

The landed BIR core is the storage premise:

- all phases use the shared flat `Node`/instruction record model;
- graph, arena, operand/value storage, and stable identities remain common
  infrastructure rather than phase-specific class hierarchies;
- stable IDs, not names, pointers, vector positions, display order, or
  analysis-local dense indices, are semantic identity;
- a lowering may preserve or replace an identity only under the normative
  identity gate, not because storage happens to be reusable.

Idea 746 and the current BIR core documentation/interface are evidence for
this premise. This idea may document proposed APIs or schemas, but it does not
modify the landed storage implementation.

### 2. NodeKind/tag and phase-vocabulary contract

Ideas 746, 801, and 802, together with
`docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md`, are the
normative authority for:

- the one-registry `NodeKind` schema and its closed six-axis tag algebra;
- compile-time and runtime stage-qualified queries;
- the distinction between static `SsaEligible` classification and B4's
  dynamic graph SSA proof;
- explicit Raw, Canonical, Prepared, PseudoPreallocation, Allocated, and
  MirReadyMachine vocabularies;
- B-to-F lowering, identity, verification, and publication rules;
- C++20 as the project host-language standard and the named NodeKind schema
  authoring form.

Phase documents and pass plans must consume that authority by reference. They
must not create a second tag table, redefine an axis, infer stage inheritance,
or claim that a helper query proves dynamic graph validity.

### 3. LIR-to-BIR importer as factual Raw boundary

The parallel LIR-to-BIR implementation is the factual producer of Raw BIR.
Planning may proceed while that route is still converging, but this idea may
describe only importer behavior that can be supported by landed source,
tests, or accepted documentation evidence.

The final closure gate requires a fresh audit after the importer work lands:

- inventory the actual published Raw node kinds, payload alternatives,
  operand/result roles, types, CFG/terminator facts, stable source identities,
  metadata, and rejection behavior;
- identify the exact private-draft and Raw publication/verifier boundary;
- reconcile the B1 admission contract and every downstream assumption with
  those facts;
- mark absent or partial behavior truthfully rather than completing the
  importer contract by speculation.

Importer code, LIR schemas, and importer tests are read-only evidence for this
idea. If the landed importer disagrees with a proposed Raw contract, Markdown
must be corrected or the disagreement recorded as a separately scoped
implementation requirement. This idea must not repair the code.

## Strict Output Boundary

All changes made while executing this idea must be Markdown files.

- Permitted: `ideas/**/*.md`, `plan.md`, `todo.md`, and BIR or adjacent
  architecture documents ending in `.md`.
- Permitted: proposed C++, schema, API, verifier, and query examples only
  inside fenced code blocks in Markdown.
- Forbidden: production or test source, headers, CMake/build files, scripts,
  generated sources, regression logs, binaries, or any other non-Markdown
  artifact.
- A proposed code block is explanatory contract text. It is not evidence that
  the API or implementation exists.

Any discovered code change becomes a separately approved implementation idea;
it is never absorbed into 732.

## In Scope

- Reconcile `src/backend/bir/README.md` and subordinate BIR Markdown with the
  landed shared-node, stable-identity, NodeKind/tag, and vocabulary contracts.
- Audit the landed importer as the Raw publication producer and make phase B's
  input contract factual.
- Plan every phase-B through phase-F pass in normative order.
- For every pass, define:
  - exact input capability, revision, stage, and admitted NodeKind vocabulary;
  - required analyses and immutable products, including revision/target keys;
  - explicit per-kind retain, replace, expand, merge, delete, or reject rules;
  - tag/vocabulary transition and forbidden residual kinds;
  - stable identity preservation/replacement decision and provenance rules;
  - output capability, stage, verifier/profile, and publication transaction;
  - analysis preservation/invalidation and earliest recomputation point;
  - diagnostics, rollback, and fail-closed behavior;
  - exact downstream consumer and adjacency proof.
- Keep all shared-node phases on the same storage model while describing
  explicit vocabulary lowering from B through F.
- Make SSA ownership precise: B4 establishes graph SSA; D5 removes phi/SSA
  form where required; later stages must not infer SSA from the static
  `SsaEligible` tag alone.
- Inventory every current `src/backend/bir/**/*.md` owner and distinguish
  normative design, scaffold, partial implementation, and complete
  implementation truthfully.

## Ordered Pass Coverage

The runbook must cover every row below. It may refine details from evidence,
but it may not silently omit or reorder a row.

### Phase B — target-independent canonicalization

- `B1/P01 legalize`: Raw vocabulary to legal target-independent forms.
- `B2/P02 scalar`: scalar, comparison, and select normalization.
- `B3/P03 cfg`: terminator, block, and edge normalization.
- `B4/P04 ssa`: dynamic SSA construction and verification, including phi
  form; `SsaEligible` remains only a static admission predicate.
- `B5/P05 memory`: memory, address, atomic, and effect-bearing forms.
- `B6/P06 aggregate`: aggregate values, copies, and projections.
- `B7/P07 intrinsics`: canonical target-independent intrinsic forms.
- `B8`: Canonical verification and atomic publication.

### Phase C — target facts and immutable preparation

- `C1`: exact `TargetProfile` selection and validation without BIR mutation.
- `C2`: BIR target-layout derivation.
- `C3`: immutable ABI preparation.
- `C4`: immutable call preparation.
- `C5`: immutable variadic preparation.
- `C6`: immutable address preparation.
- `C7`: immutable inline-assembly target context.
- `C8`: immutable runtime-helper preparation and cumulative publication.
- `C9`: register-constraint parsing, typing, binding, and exact-revision
  projection authority.

Phase C does not silently mutate Canonical nodes. Documents must distinguish
the Canonical graph from exact-revision, target-keyed preparation products and
must explain how those products admit D1.

### Phase D — pseudo formation and pre-allocation legalization

- `D1`: generic pseudo lowering into a new admitted revision.
- `D2`: shared ABI-aware BIR call lowering.
- `D3`: Pseudo verification and atomic publication.
- `D4`: target-specific pseudo legalization/expansion and full reverification.
- `D5`: out-of-SSA and owned copy-resolution closure.

Documents must enumerate Canonical/Prepared kinds that are retained or
replaced, the pseudo kinds introduced, the exact point where phi/SSA form is
removed, and how every introduced value enters later liveness/allocation.

### Phase E — allocation and MIR-ready publication

- `E1`: allocation liveness and interference.
- `E2`: shared pseudo-physical register allocation.
- `E3`: explicit spill/reload insertion and the bounded retry to E1.
- `E4`: copy closure, frame-action materialization, exact-current product
  recomputation, Allocated verification, and MIR-ready publication.

Documents must distinguish analysis products from graph revisions, describe
retry invalidation precisely, enumerate allocation/frame kinds and tags, and
prove no unresolved expansion or hidden frame work crosses E4.

### Phase F — machine graph and emission

- `F1`: strict machine-graph construction from the verified MIR-ready view.
- `F2`: machine verification without allocation or legalization repair.
- `F3`: assembly, object, relocation, and link emission boundaries.

Documents must define the final BIR-to-machine vocabulary transition. Machine
identity is distinct; a BIR stable ID may be retained only as provenance.

## Per-Pass Markdown Contract

Each pass owner must present its contract in a consistent, agent-readable
order:

1. `Purpose`
2. `Owns`
3. `Does Not Own`
4. `Inputs`
5. `Input NodeKind/Tag Vocabulary`
6. `Required Analyses and Products`
7. `Ordered Behavior`
8. `NodeKind/Tag Lowering Matrix`
9. `Identity and Provenance`
10. `Outputs`
11. `Verification and Publication`
12. `Analysis Preservation and Invalidation`
13. `Failure and Diagnostics`
14. `Adjacent-Stage Contract`
15. `Implementation State`
16. `Proof Requirements`
17. `Open Questions`

The lowering matrix must contain explicit rows. A catch-all statement such as
"all other nodes pass through" is insufficient unless the admitted input set
is closed and every member is mechanically accounted for by a referenced
normative table.

Suggested APIs may be shown only as Markdown examples, for example:

```cpp
// Documentation sketch only; not an implementation claim.
switch (node.kind) {
case NodeKind::Example:
  return retain_or_lower(node);
default:
  return fail_unhandled_kind(node.kind);
}
```

## Cross-Phase Rules

- The order remains `Raw -> B -> C -> D -> E -> F`; local documents cannot
  reorder it.
- Every stage consumes the exact verified capability published by its
  predecessor. Adjacency cannot be established by assertion alone.
- All semantic graph mutations create private candidates and publish
  atomically after the stage verifier succeeds.
- Preparation and analysis products are immutable, exact-revision keyed facts;
  they do not become a second semantic graph authority.
- NodeKind admission is explicit per published stage. Shared storage never
  implies that a kind survives into the next vocabulary.
- A NodeId is preserved only when the 801 identity-preservation conditions all
  hold. Semantic, result/type, operand-role, effect, ownership, or publication
  changes require replacement and explicit provenance.
- Unknown kinds, unknown tags, incomplete lowering matrices, stale products,
  and unhandled payload alternatives fail closed.
- `E3 -> E1` is the only ordinary retry edge described by the current root
  contract; it does not publish an intermediate capability.
- F1 consumes MIR-ready facts apply-only. It cannot perform hidden expansion,
  ABI lowering, register allocation, spilling, or frame repair.

## Out of Scope

- Any non-Markdown implementation or test change.
- Redesigning the shared Node/graph/arena/stable-ID storage model.
- Redefining the 801 six-axis algebra, duplicating its registry, or replacing
  its pass-facing helpers.
- Changing LIR semantics or modifying the LIR-to-BIR importer.
- Claiming a proposed API, pass, verifier, target table, or machine lowering is
  implemented merely because Markdown contains a code block.
- Activating a downstream implementation proposal.
- Weakening tests, expectations, unsupported classifications, or failure
  contracts to make documentation appear converged.

## Acceptance Criteria

- Every B1-B8, C1-C9, D1-D5, E1-E4, and F1-F3 row has an authoritative
  Markdown owner and a complete per-pass contract.
- Every pass has a closed input/output NodeKind and tag vocabulary, explicit
  lowering matrix, identity policy, verifier/publication gate, dependencies,
  invalidation, failure behavior, and downstream acceptance evidence.
- Phase documents consume the 746/801/802 contracts without creating a second
  classification or stage authority.
- B4 and D5 state the exact dynamic SSA establishment/removal responsibilities;
  no document treats a static tag query as graph validity.
- The final importer audit is performed against the landed LIR-to-BIR code and
  tests, and Raw/B1 admission facts are reconciled without speculation.
- Every current BIR Markdown file is indexed and its implementation status is
  truthful; missing real owners receive Markdown placeholders only.
- Cross-phase adjacency, revision/target binding, stable identity, failure
  atomicity, verifier ownership, and analysis invalidation are consistent from
  Raw publication through F3.
- The complete accepted diff contains Markdown files only. Any fenced C++ is
  visibly labeled as a proposal or contract sketch.

## Closure Gate

Runbook exhaustion alone does not close 732. Closure requires:

1. all B-F pass contracts and the cross-phase audit are complete;
2. the parallel LIR-to-BIR work has landed at an identifiable revision;
3. its actual Raw publication behavior has been audited and reconciled with
   B1 admission;
4. remaining code/document disagreement is either corrected in Markdown as
   implementation truth or recorded as a separately scoped open idea;
5. a Markdown-only diff audit confirms no production, test, build, generated,
   log, or script artifact changed.

## Reviewer Reject Signals

- Reject any non-Markdown change, including implementation added to make a
  proposed pass contract true.
- Reject C++ or schema text outside a fenced Markdown code block, or a code
  block presented as proof of implementation.
- Reject stale language that parks 732 behind idea 734 or assumes the importer
  is complete without auditing its landed behavior.
- Reject redefining NodeKind tags, stage admission, SSA eligibility, or
  identity rules instead of consuming the 801 normative contract.
- Reject an omitted B-F row, a catch-all lowering rule over an unenumerated
  vocabulary, or a pass that silently retains kinds across a stage boundary.
- Reject treating `SsaEligible` as proof that a graph is in SSA form.
- Reject identity preservation justified only by shared storage, equal payload
  layout, pointer equality, or convenience.
- Reject adjacent-stage acceptance by assertion without matching producer and
  consumer vocabulary, revision, product, verifier, and failure contracts.
- Reject documentation that claims absent or partial production code is
  complete.
- Reject testcase-shaped contracts, named-case-only rules, expectation
  downgrades, allowlist changes, or weaker failure behavior presented as
  architecture progress.
- Reject broad formatting churn, helper renames, or classification-only edits
  claimed as complete pass planning while ownership, vocabulary, identity,
  publication, or failure seams remain unresolved.

## Resumption Record — switched to idea 803

Idea 732 remains incomplete and resumable. Steps 1 through 8A are accepted;
Step 8B was interrupted before execution:

- `Current Step ID: 8B`
- `Current Step Title: Reconcile the landed importer and decide closure`
- Last accepted result: all 47 current BIR Markdown paths and all 31 A1-F3
  rows were audited, the 26 pass owners were converged to the adapted
  17-section contract, and every non-importer cross-phase seam was closed.
- Accepted documentation history: `dbff4a028`, `3dda842fc`, `820c1c633`,
  `e495edf69`, `e7882e727`, `518499625`, `9a79261c6`, `670d6ff24`,
  `07761ce16`, `dd1c535e7`, `07c0c3dd0`, `3d7c9dd92`, and `1e01e8b8a`.
- Accepted proof: Markdown-only changed-path checks, 47/47 document inventory,
  31/31 ordered A1-F3 rows, 26 exact adapted pass spines, 166 relative links
  checked with zero broken links, focused authority/status searches, and
  `git diff --check`.
- Waiting condition: the importer completion gate remains unmet.
  `origin/new_bir@13123e7524c12a307bae372007be60e0a4e656a3` still identifies
  active blocker idea 763 rather than a named completed importer revision with
  inspectable code and matching test/acceptance proof. This wait is outside
  732's Markdown-only execution scope and is not evidence that 732 is closed.
- Exact return action: once such a completed importer revision exists, audit
  the landed Raw NodeKind/payload/operand/result/type, graph/CFG/order,
  identity/metadata, builder publication/rollback, verifier, accepted/rejected
  LIR, and absence-of-later-stage facts; reconcile A1/A2/B1 and any downstream
  effects; then request an explicit closure decision for idea 732.

Until that gate is met, do not guess importer facts, weaken the closure gate,
or redo accepted Steps 1 through 8A.
