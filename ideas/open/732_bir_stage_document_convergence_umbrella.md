# BIR Stage Documentation Convergence Umbrella

Status: Open (parked pending idea 734)
Type: Umbrella triage and ordered follow-up idea generator
Parent: none
Handoff Directory: none; the durable handoff is this umbrella plus exactly six
ordered child source ideas under `ideas/open/`
Related:
- `src/backend/bir/README.md`
- `ideas/closed/731_inline_asm_transport_and_regalloc_contract.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/draft/733_accepted_bir_a1_f3_architecture_implementation.md`

## Lifecycle Reconciliation

The user reactivated this docs-only umbrella after moving the general A1-F3
implementation proposal to `ideas/draft/`. The earlier monolithic documentation
checkpoint did not execute this umbrella's prescribed six-child A-through-F
workflow, so the ownership/order/adjacency proof remains incomplete.

Idea 731 is closed only for its bounded structured inline-asm transport proof.
Observed production-importer and new-BIR container gaps are recorded in idea
734. Existing LIR is authoritative and complete for this route;
README `source gap` labels are documentation/importer assumptions to audit, not
permission to change LIR. The sole possible LIR-schema exception is a minimal,
evidence-required inline-asm constraint carrier that records reviewed
requirements against ordinary operand/result positions and roles. The former
requirement that this umbrella settle all phases before idea 734
could activate is superseded by the user's explicit source intent. Idea 734 is
now the active C++ implementation initiative. This umbrella and its children
remain open but parked until 734 closes. Draft idea 733 remains parked and
supplies no implementation authority.

### Post-734 Restart Authority

After idea 734 is implemented, accepted and closed, reactivate this umbrella
and rerun documentation convergence from phase A, then B, then C and onward
against the landed implementation. Do not resume directly at Child C or its
old Step 7 checkpoint. Preserve the already-landed phase-A/B documentation
acceptances and phase-C C1-C6 slices as historical evidence, but re-execute and
revalidate those phase contracts before they can serve as current post-734
acceptance gates.

## Goal

Classify and converge the complete BIR documentation architecture by generating
exactly six documentation-only child source ideas, one for each normative root
phase `A` through `F`. The child ideas must review and edit Markdown contracts
strictly in the order declared by `src/backend/bir/README.md`, prove every input
and output seam, and leave no missing, duplicated, or misleading owner. On the
post-734 rerun they must reconcile those contracts with the landed new Raw-BIR
container/import implementation.

This umbrella performs classification and planning only. It does not repair a
BIR document, modify implementation, or activate an implementation runbook.

## Why This Exists

The root BIR overview now describes a multi-stage architecture spanning source
import, target-independent canonicalization, target-derived preparation,
pseudo formation, shared allocation, and a strict MIR/emission boundary. That
architecture cannot be implemented safely while subordinate documents differ
in format, omit an owner, describe incomplete input shapes, publish an output
that its consumer cannot accept, duplicate authority, or claim scaffolded code
already exists.

A single broad documentation pass would make it too easy to answer local
implementation questions before resolving ownership and adjacency. Six ordered
phase ideas make each boundary reviewable while preserving the root order and
forcing every downstream phase to consume the contract accepted immediately
before it.

## Current Evidence

- `src/backend/bir/README.md` is the normative architecture overview, ordered
  stage/pass index, verifier-profile summary, analysis dependency index, and
  complete-documentation review order. Its phase definitions and within-phase
  order are the source of truth for child generation.
- The accepted root/subordinate documentation records an architecture
  direction, but the phase-by-phase ownership and adjacency proof required by
  this umbrella has not been executed. Draft idea 733 is not current
  implementation authority; closed idea 731 proves only one bounded transport
  path.
- Idea 734 records a phase-A receiving-boundary gap: production import accepts
  only a small module/function/instruction/terminator slice and new BIR does not
  yet expose complete typed containers and wiring for the existing complete LIR
  surface. README `source gap` rows must be audited as potentially stale
  assumptions about LIR, not repeated as established LIR defects.
- Current Markdown status labels and section shapes are not yet a sufficient
  proof of contract completeness. A later child must inspect the actual file,
  its producer, its consumer, and current implementation truth rather than
  accepting a checkbox or heading-only conversion.

The original umbrella route would have treated its six children as
documentation-only. The lifecycle reconciliation above supersedes that
unexecuted routing proposal; it does not retroactively claim the children
existed or that this umbrella met its acceptance criteria.

## In Scope

- Generate exactly six child source ideas under `ideas/open/`, with phase names
  and dependency order fixed as `A -> B -> C -> D -> E -> F`.
- Assign child idea IDs at generation time from the then-available namespace;
  do not assume that IDs `733` through `738` will remain available.
- In every child, inventory every Markdown owner assigned to that phase by the
  root README, including relevant stage, pass, analysis, schema, boundary, and
  support documents, then detect any missing owner file or required placeholder.
- Review and edit the phase's documents in the precise stage/pass order from
  the root README.
- Normalize each reviewed document to the uniform metadata and core-first
  contract format below, using the permitted analysis/support variant where
  appropriate.
- Build explicit input-coverage and output-handoff matrices for every reviewed
  Markdown contract.
- Prove revision and target binding, stable identity, failure atomicity,
  verifier/profile ownership, analysis invalidation, unique authority, and
  truthful implementation status at every applicable boundary.
- Inspect adjacent producer and consumer contracts even when only the current
  phase owns edits. Use a coordinated boundary packet naming both owners when
  an accepted repair must cross a phase boundary.
- After all six children converge, perform one umbrella-level cross-phase audit
  proving every current `src/backend/bir/**/*.md` file is indexed, formatted,
  adjacent-compatible, and truthful about implementation state.
- On the post-734 rerun, make phase A revalidate the implemented 734 handoff:
  inventory every existing LIR variant/metadata fact, name the new Raw-BIR
  receiving container and importer rule, identify new-BIR-only gaps, and
  require a complete coverage matrix. LIR files, schemas, and producers are
  immutable external input except for the sole evidence-gated minimal inline-asm
  constraint-carrier exception; inline-asm values remain ordinary SSA
  operands/results and asm text remains opaque and byte-exact.

## Out Of Scope

- Repairing BIR Markdown files directly in this umbrella.
- C++ implementation, build wiring, tests, runtime behavior, target backend
  implementation, or any other non-documentation architecture change in this
  umbrella or its six children.
- Activating draft idea 733 or another downstream implementation idea while
  this documentation workflow is unresolved. Idea 734 is the explicit
  sequencing exception and active prerequisite to the restarted workflow.
- Silently changing the normative phase or stage order.
- Mixing ownership from unrelated phases in one child or treating a child as a
  general backend cleanup route.
- Expectation rewrites, unsupported-marker changes, allowlist changes,
  testcase-shaped shortcuts, or weaker proof contracts presented as progress.
- Starting implementation outside idea 734 before the restarted six-phase
  reviews and final cross-phase audit are accepted.

## Uniform Document Contract

Every reviewed BIR Markdown owner must begin with a common metadata spine. The
two status axes and their values are exact:

```text
Contract-Status: scaffold | under-review | accepted
Implementation-Status: absent | partial | complete
Kind: <stage | pass | analysis | schema | boundary | support | audit>
Phase-ID: <root phase/stage identifier>
Upstream: <producer contract or none>
Downstream: <consumer contract or none>
Owner-Path: <repository path>
Last-Reconciled-Commit: <commit or none>
```

Cross-cutting analysis, support, and audit documents that do not execute as one
linear stage may use `Applies-To` instead of `Phase-ID`. They still require the
same two status axes, `Kind`, `Upstream`, `Downstream`, `Owner-Path`, and
`Last-Reconciled-Commit`.

Every stage/pass/schema/boundary document must answer the core contract in this
order, before API, algorithm, or implementation discussion:

1. `Purpose`
2. `Owns`
3. `Does Not Own`
4. `Inputs`
5. `Outputs`
6. `Adjacent-Stage Contract`

Only after those core questions are answered may the document contain these
implementation/detail sections, in this order when applicable:

1. `Ordered Behavior`
2. `Invariants`
3. `Verification and Publication`
4. `Failure and Diagnostics`
5. `Analysis and Invalidation`
6. `Target and ABI Rules`
7. `Implementation State`
8. `Proof Requirements`
9. `Open Questions`
10. `Review Checklist`

Analysis and support documents may adapt the detail headings to their
non-mutating role, but must preserve the same metadata and the ownership,
input, output, adjacency, failure, implementation-truth, and review spine.
Analysis variants must make cache/revision keys, earliest consumers, derived
facts, invalidation, and stale-result rejection explicit. Support variants
must identify the authoritative facts they render or audit and must not become
a second semantic owner.

Heading presence is not conformance. The text under each heading must resolve
the relevant contract questions completely and consistently with both adjacent
documents.

## Required Per-Document Review Method

Each child idea must require these actions for every Markdown file it assigns:

1. Check the file against the uniform format and its declared kind.
2. Edit the file until its metadata, core contract, applicable detail sections,
   and implementation truth conform; do not stop at inserting headings or
   checking boxes.
3. Detect missing owner files and placeholders implied by the root order,
   inputs, outputs, verifier gates, analyses, schemas, or boundaries. Create a
   documentation-only placeholder when the owner is real but implementation is
   absent, and index it coherently.
4. Exhaustively enumerate, in an input-coverage matrix, every artifact and
   meaningful variant arriving from each input/producer. For each row, identify
   its producer, revision/target key, stable identities, validation state,
   optional/error forms, and the receiving field or rule that proves the
   current document can carry it without loss or guesswork.
5. Exhaustively enumerate, in an output-handoff matrix, every artifact and
   meaningful variant the owner publishes or guarantees. For each row,
   identify its exact downstream consumer, revision/target binding,
   publication/verifier gate, failure behavior, invalidated analyses, and the
   downstream Markdown clause that proves acceptance. An assertion that the
   consumer can accept an output is not evidence.

Every child must resolve duplicate authority and make failure atomicity
explicit: rejected private candidates publish no capability, partial revision,
stale analysis, or misleading readiness token. Stable semantic identity must
never be inferred from names, pointers, vector positions, display order, or
analysis-local dense indices.

The child must answer ownership, full input coverage, complete output shape,
and adjacent acceptance before proposing API names, algorithms, data layouts,
or implementation steps. Large implementation sketches are grounds for
revision when they conceal an unresolved core seam.

## Priority Model

1. After 734 closes and this route restarts, follow the normative root order
   exactly: finish and accept phase `A` before phase `B`, then `C`, `D`, `E`,
   and `F`.
2. Within a phase, follow the root README's stage/pass order. Review a
   dependency analysis immediately before its earliest normative consumer when
   the root classifies it as on-demand rather than a linear stage.
3. Resolve first-owner and adjacent-boundary ambiguity before formatting polish
   or implementation detail.
4. Prefer lossless coverage of all producer variants and explicit downstream
   evidence over novelty, file count, or named testcase pressure.
5. A stage-order change is never a local cleanup. If evidence requires one,
   update the root README and both adjacent contracts coherently in one
   explicitly coordinated documentation boundary packet, then regenerate the
   affected review order.
6. Do not begin the next child while the previous child's output contract is
   unaccepted; every later child consumes the accepted predecessor contract.

## Required Follow-Up Ideas

Generate exactly these six child ideas. Their numeric IDs are chosen only when
they are created, but their phase names, order, scope, and dependencies are
fixed.

Idea 734 is the active implementation prerequisite, not one of the six
documentation children. The earlier Child-A contract is historical input to
734. After 734 closes, Child A must be re-executed first and reconcile the
landed implementation before Child B may run; it must not implement additional
734 scope or change LIR beyond the sole evidence-proven minimal inline-asm
constraint-carrier exception.

### Child A — Import and Raw Publication

- Owns the documentation convergence route for phase A from typed LIR import
  through Draft/Raw verification and `RawBir` publication.
- Inventories all root-assigned phase-A owners and relevant import/core/schema,
  verifier, diagnostic, analysis, and boundary contracts.
- Establishes the complete output contract consumed by phase B.
- Has no child-phase dependency; it consumes the external typed-LIR contract.
- Treats existing LIR as complete, corrects stale documentation assumptions,
  and revalidates the landed new-BIR container/import handoff implemented by
  idea 734. Its only possible LIR-schema exception is the minimal inline-asm
  constraint carrier described above; no special inline-asm value model,
  allocator, projection machinery, or assembler parsing is permitted.

### Child B — Target-Independent Canonicalization

- Owns the documentation convergence route for the ordered `P01` through `P07`
  canonical passes and Canonical verification/publication.
- Consumes the accepted phase-A `RawBir` output contract.
- Inventories all root-assigned phase-B pass, pipeline, analysis, verifier,
  schema, publication, and support owners relevant to canonicalization.
- Establishes the complete target-independent `CanonicalBir` contract consumed
  by phase C.

### Child C — Target Facts and Immutable Preparation

- Owns target-profile consumption, BIR target-layout derivation, all ordered
  immutable preparation products, and register-constraint typing/binding as
  assigned by the root README.
- Consumes the accepted phase-B `CanonicalBir` contract.
- Inventories every phase-C target/layout/preparation/constraint owner plus
  relevant analysis, verifier, schema, product-key, and publication documents.
- Establishes the complete verified target/preparation bundle consumed by
  phase D without mutating Raw or Canonical BIR.

### Child D — Pseudo Formation and Pre-Allocation Legalization

- Owns generic pseudo lowering, shared ABI-aware call lowering, Pseudo
  verification, target-specific pseudo legalization/expansion, and out-of-SSA
  documentation in their root-declared order.
- Consumes the accepted phase-C target/preparation and constraint contracts.
- Inventories all phase-D pseudo schema/pass/verifier/analysis/boundary owners,
  including the ABI-aware call-lowering seam and any allocation-aware
  out-of-SSA continuation explicitly assigned by the root contract.
- Establishes the complete directly realizable pre-allocation pseudo-BIR
  contract consumed by phase E.

### Child E — Shared Allocation and MIR-Ready Publication

- Owns allocation liveness/interference, shared pseudo-physical allocation,
  explicit spill/reload and retry behavior, copy-resolution handoff where
  assigned, and atomic Allocated/MIR-ready verification/publication.
- Consumes the accepted phase-D pseudo-BIR output contract.
- Inventories all phase-E allocation, liveness, constraint-use, spill/reload,
  allocated schema/view, verifier, invalidation, and publication owners.
- Establishes the complete `MirReadyBirView` contract consumed by phase F.

### Child F — Strict One-to-One MIR and Emission Boundary

- Owns documentation convergence for strict one-pseudo-node to one-machine-
  record MIR construction, machine verification, and assembly/object/link
  emission boundaries.
- Consumes the accepted phase-E `MirReadyBirView` contract.
- Inventories all root-assigned phase-F BIR-facing MIR, target mapping,
  verifier, inline-asm late parsing, object, emission, and support boundaries.
- Proves MIR does not expand calls/instructions, allocate registers, create
  allocatable temporaries, or repair pressure, and that every terminal output
  reaches an explicitly documented consumer.

Dependency is strict: B consumes A, C consumes B, D consumes C, E consumes D,
and F consumes E. Each child inspects its upstream producer and downstream
consumer, but edits only owners declared within its phase unless a coordinated
boundary packet explicitly names both phase owners and the exact shared seam.

## Completed Runbook Handoff

The initial umbrella runbook completed its queue-generation purpose and was
retired after the supervisor's Step-4 audit accepted this strict docs-only
sequence:

1. Child A: `ideas/closed/735_bir_phase_a_import_raw_document_convergence.md`
2. Child B: `ideas/closed/736_bir_phase_b_canonical_document_convergence.md`
3. Child C: `ideas/open/737_bir_phase_c_preparation_document_convergence.md`
4. Child D: `ideas/open/738_bir_phase_d_pseudo_document_convergence.md`
5. Child E: `ideas/open/739_bir_phase_e_allocation_document_convergence.md`
6. Child F: `ideas/open/740_bir_phase_f_mir_boundary_document_convergence.md`

The eventual acceptance order remains strictly `A -> B -> C -> D -> E -> F`,
but it restarts only after 734 closes. Children A and B were accepted by proofs
`2ec50b3e` and `769f0d012`, and Child C landed C1-C6 work; all are preserved as
pre-734 historical evidence and require post-734 re-execution/revalidation.
Idea 734 is now active, Child C is parked rather than complete, and draft idea
733 remains parked with no implementation authority.
The C1 external Markdown boundary question belongs to Child C; the F2 verifier and F3
assembler/object/link Markdown-authority or placeholder questions belong to
Child F.

This idea is **not closed**. It remains open while all six children execute in
order and until the final umbrella-level cross-phase Markdown inventory,
adjacency, implementation-truth, and failure/invalidation audit is accepted.
The accepted queue-generation checkpoints were `8ba7a8f3`, `b4ab5b79`,
`9c28f521`, and `aae105de`; they authorize documentation lifecycle handoff
only, not implementation.

### Child-A Acceptance Checkpoint

Child A completed the three-owner and coordinated shared-boundary
documentation route in `10d70b872`, `060a32c78`, `e759322a`, `237afcdf`,
`ba1dcab8`, `65a20c2d`, and final proof `2ec50b3e`. Its exact
`ModuleDraft -> A2 -> RawBir -> B1` handoff is accepted without claiming
missing implementation complete or changing LIR. Child B was the authorized
successor under the then-current sequence. This checkpoint remains historical
evidence and must be revalidated after 734 before a new Child-B acceptance.

### Child-B Acceptance Checkpoint

Child B completed its seven analysis owners, P01-P07 pass owners and B8
framework/pipeline/Canonical-verifier boundary in `c7bb43d3e`, `ce3dacf3f`,
`878e56a97`, `5eb4d6f43`, `b6cabf1d2`, `d4c73bdf4`, `c0e3cdc6e`,
`938c7b43e`, `7b01fd0eb`, and final proof `769f0d012`. Its exact verified
`CanonicalBir -> C1` handoff is accepted without claiming absent/partial
implementation complete or authorizing target-aware work. Child C is the only
authorized successor under the then-current sequence. Idea 734 now supersedes
that sequence; this checkpoint remains historical evidence and must be
revalidated after 734 before Child C restarts.

## Acceptance Criteria

- Exactly six ordered child source ideas exist across lifecycle storage, one
  for each fixed phase A through F, with collision-safe IDs assigned at
  generation time and explicit predecessor/successor dependencies.
- Each child is a documentation-only architecture review/edit route and embeds
  the uniform format, per-document review method, phase inventory, missing-file
  detection, input-coverage matrix, and output-handoff matrix requirements.
- Every child follows the stage/pass order in `src/backend/bir/README.md` and
  answers core ownership/input/output/adjacency questions before implementation
  detail.
- Phase A is accepted before B, B before C, C before D, D before E, and E
  before F. No child accepts its predecessor's output by assertion alone.
- Any necessary root-order correction is made only through a coordinated
  documentation packet that updates the root and both adjacent contracts.
- After all six children are accepted, a final umbrella-level audit inventories
  every current `src/backend/bir/**/*.md` file and proves each is indexed
  exactly once where appropriate, format-conformant, uniquely owned,
  adjacent-compatible, revision/target-safe, failure-atomic, invalidation-
  complete, and truthful about actual implementation state.
- The umbrella and children make no implementation, test-expectation,
  unsupported, allowlist, or runtime behavior changes. Idea 734 is the explicit
  preceding C++ implementation initiative; no other implementation begins
  until the restarted final audit is accepted.
- The final audit records any deliberate external owner referenced from BIR
  docs and verifies its BIR-facing contract without silently absorbing that
  external implementation into a child.
- Idea 734 is linked as the new-BIR container/schema plus importer
  implementation prerequisite for the unchanged LIR surface. It must close
  before this umbrella restarts from phase A.

## Closure Note Requirements

The closure note must record:

- the final numeric IDs and paths assigned to children A through F
- confirmation that they were executed and accepted in strict A-to-F order
- the root README revision used as the normative ordering evidence, including
  any explicitly coordinated order corrections
- the complete final Markdown inventory, missing files/placeholders created,
  duplicate authorities removed, and unresolved or intentionally external
  owners
- where each phase's input-coverage and output-handoff matrices live
- the final cross-phase adjacency, revision/target binding, stable-identity,
  verifier/profile, failure-atomicity, invalidation, and implementation-truth
  audit result
- any remaining disagreement between accepted documentation and current code;
  such disagreement must be called out explicitly and blocks implementation
  authorization unless the documentation truthfully marks it absent/partial
- explicit confirmation that neither the umbrella nor its children performed
  implementation work or weakened tests/expectations
- whether architecture documentation convergence is accepted and whether a
  separate implementation idea may now be proposed; closure itself does not
  activate implementation

## Reviewer Reject Signals

- Reject a missing Markdown owner, an owner absent from the root index, or two
  documents claiming the same semantic/publication authority.
- Reject checkbox-only or heading-only format compliance without substantive
  ownership, input, output, adjacency, failure, and implementation-truth text.
- Reject an input matrix that omits producer variants, error/optional forms,
  revision/target keys, stable identities, or the receiving schema/rule.
- Reject an output claimed acceptable by assertion when the downstream
  Markdown contract does not explicitly demonstrate that it can consume the
  exact artifact and variants.
- Reject API, algorithm, data-structure, or large implementation detail placed
  before unresolved Purpose/Owns/Does-Not-Own/Inputs/Outputs/adjacency seams.
- Reject a silent stage reorder or a local edit that changes order without
  coherently updating the root README and both adjacent contracts.
- Reject mixed phase ownership in one child unless a narrowly coordinated
  boundary packet explicitly names both owners and the seam being repaired.
- Reject documentation claiming an API, pass, verifier, build edge, target
  table, or implementation exists when the repository only contains a
  scaffold, placeholder, partial path, or no code.
- Reject direct C++ implementation, build/test behavior changes, or target
  backend implementation inside this umbrella or any of its six children.
- Reject testcase-shaped shortcuts, named-case-only contracts, unsupported
  expectation downgrades, allowlist filtering, weaker test contracts, or
  expectation rewrites presented as architecture progress.
- Reject helper renames, classification-only changes, formatting churn, or a
  new abstraction name that preserves the same missing ownership/input/output
  failure while being claimed as convergence.
- Reject resuming Child C directly after 734, beginning Child B through F
  before its post-734 predecessor contract is accepted, or beginning any
  implementation outside 734 before all restarted children and the final
  cross-phase audit converge.
