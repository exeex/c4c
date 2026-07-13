# BIR Documentation Ownership And Seam Convergence Runbook

Status: Active
Source Idea: ideas/open/732_bir_stage_document_convergence_umbrella.md
Activated from: the user-restored docs-only umbrella after ideas 733 was moved
to draft and idea 734 was recorded as a deferred phase-A implementation
consumer

## Purpose

Settle the Markdown ownership, ordering, and producer/consumer seams for the
complete BIR route before any additional new-BIR implementation is authorized.

## Goal

Produce an exact ordered documentation-review queue for phases A through F,
with phase A explicitly proving how the unchanged complete LIR surface maps to
new Raw-BIR containers and the later idea-734 importer boundary.

## Core Rule

This runbook is documentation classification and lifecycle planning only. It
may inventory Markdown, compare contracts, and create ordered documentation
child ideas. It must not edit C/C++ implementation, tests, build files, LIR,
runtime behavior, unsupported expectations, or any downstream implementation
proposal.

Existing LIR is authoritative and complete. A `source gap` label in current
BIR documentation is an assumption to audit, not permission to change LIR.
The sole possible LIR-schema exception is a minimal evidence-required inline-
asm constraint carrier that records reviewed requirements against ordinary
operand/result positions and roles. Missing receiving state otherwise belongs
to new-BIR documentation or the future idea-734 importer wiring.

## Read First

- `ideas/open/732_bir_stage_document_convergence_umbrella.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` as a deferred,
  inactive phase-A implementation consumer
- `ideas/closed/731_inline_asm_transport_and_regalloc_contract.md` as bounded
  historical proof only
- `ideas/draft/733_accepted_bir_a1_f3_architecture_implementation.md` as a
  parked proposal with no implementation authority
- `src/backend/bir/README.md`
- `src/backend/bir/lir_to_bir/README.md`
- `src/backend/bir/LEGACY_COVERAGE.md`
- `src/backend/bir/REVIEW_TEMPLATE.md`

## Scope

- inventory every current BIR Markdown owner and the root-declared phase/order
- classify each document as stage, pass, analysis, schema, boundary, support,
  or audit owner
- identify missing owners, duplicate authority, stale implementation claims,
  and producer/consumer seams that are asserted rather than demonstrated
- define input-coverage and output-handoff matrix requirements per phase
- define phase A against every existing LIR variant and metadata family; keep
  LIR immutable except for the sole inline-asm constraint-carrier exception
- create exactly six ordered documentation-only child ideas A through F with
  strict dependencies and collision-safe IDs
- link phase A to deferred idea 734 without activating or implementing it
- prepare the lifecycle handoff to the phase-A child while keeping umbrella
  idea 732 open until all children and the final cross-phase audit complete

## Non-Goals

- no C/C++ implementation, test, build, runtime, expectation, unsupported, or
  allowlist changes
- no LIR edit outside the evidence-proven minimal inline-asm constraint-carrier
  exception; no separate inline-asm value subsystem or binding table
- no idea-734 implementation and no activation of draft idea 733
- no canonical-pass, target, preparation, constraint, ABI, out-of-SSA,
  allocation, spill/reload, MIR-ready, MIR, emission, or late-assembly work
- no stage reorder without a separately identified coordinated documentation
  boundary packet
- no heading-only, checkbox-only, formatting-only, or testcase-shaped claim of
  documentation convergence

## Working Model

The umbrella classifies and creates the queue; it does not execute the child
documentation repairs inside this runbook. Each child must later own only its
phase's Markdown and must be activated in A-to-F order. Exhausting this runbook
does not close idea 732: the umbrella remains open until all six children and
the final audit satisfy its source criteria.

## Execution Rules

1. Use `src/backend/bir/README.md` as the initial order hypothesis, then verify
   every indexed owner and adjacency rather than accepting headings as proof.
2. Keep all work documentation-only. Stop if a packet requires code or test
   behavior changes.
3. Treat LIR as a complete producer boundary. Correct BIR docs that misclassify
   existing LIR facts as missing. The only possible schema exception is the
   minimal inline-asm constraint carrier defined above.
4. Require each child to enumerate producer variants, stable identities,
   revision/target keys, optional/error forms, receiving fields, verifier
   gates, failure behavior, invalidation, and exact downstream acceptance.
5. Keep idea 734 inactive. Phase A may define its prerequisite contract but
   cannot implement new-BIR containers or importer wiring.
6. Keep idea 733 under `ideas/draft/`; do not derive execution packets from it.
7. Do not close idea 732 when this runbook hands off to Child A. Record the
   generated queue and deactivate/switch lifecycle state through plan-owner.

## Ordered Steps

### Step 1 - Revalidate the root Markdown inventory and phase order

Goal: establish the complete documentation owner set and the exact proposed
A-to-F review order without changing implementation.

Actions:

- enumerate every current `src/backend/bir/**/*.md` owner plus BIR-facing MIR
  and external boundary documents referenced by the root
- map each owner to one phase/kind and record missing-index, missing-file,
  duplicate-authority, stale-status, and adjacency questions
- compare root order with each producer/consumer declaration and flag every
  seam that lacks evidence
- distinguish current implementation truth from accepted architecture text

Completion check:

- one review inventory accounts for every relevant Markdown owner exactly once
  or records an explicit cross-cutting/external classification; unresolved
  order/ownership questions are concrete enough to assign to a child

### Step 2 - Define phase A and the deferred idea-734 handoff

Goal: make the typed-LIR-to-Raw-BIR documentation boundary exhaustive and
prevent stale `source gap` language from redirecting work into LIR.

Actions:

- inventory every existing `LirInst`, `LirTerminator`, and relevant module,
  function, type, global, value, object, initializer, symbol, and metadata fact
  as external phase-A inputs
- require phase-A input and output matrices to name the typed new-BIR receiving
  container, importer disposition, verifier gate, and failure behavior for
  every row
- classify each current gap as missing new-BIR documentation/container,
  missing importer wiring, stale documentation, or already-proved coverage
- state explicitly that LIR is complete and immutable and that unsupported
  diagnostics alone cannot prove receiving completeness
- document inline-asm inputs/results as ordinary SSA values, reviewed `r`,
  `=r`, `f`, `VR`, and other evidenced requirements against ordinary positions
  and roles, and opaque byte-exact asm text; forbid special allocation,
  projection machinery, and assembler parsing
- bind deferred idea 734 to the accepted phase-A contract without activating it

Completion check:

- the phase-A child contract audits every existing LIR fact with no catch-all
  and gives idea 734 a strict new-BIR-container/importer boundary plus only the
  evidence-gated inline-asm carrier exception

### Step 3 - Create the ordered A-through-F documentation child queue

Goal: generate exactly six independently reviewable documentation ideas with
strict predecessor/successor dependencies.

Actions:

- assign collision-safe IDs and paths to Child A through Child F
- copy the umbrella's uniform document contract, required review method, and
  phase-specific owner inventory into each child
- make B consume accepted A, C consume B, D consume C, E consume D, and F
  consume E
- require every child to reject direct implementation, mixed ownership,
  assertion-only handoffs, and untruthful implementation-status claims
- keep idea 734 separate from the six docs children and keep idea 733 in draft

Completion check:

- exactly six open documentation child ideas exist, each has one phase owner,
  explicit dependencies, acceptance criteria, closure requirements, and
  concrete reviewer reject signals

### Step 4 - Audit the queue and hand off to Child A

Goal: verify that the generated route is executable without authorizing code.

Actions:

- check the six children against the umbrella inventory, phase order, matrix
  contract, external-owner rules, and final-audit requirements
- verify Child A is the only eligible next activation and that ideas 734 and
  draft 733 remain inactive
- record durable child IDs/paths and unresolved ownership questions in idea
  732 at the lowest appropriate lifecycle layer
- ask plan-owner to deactivate this runbook and activate Child A; do not close
  idea 732

Completion check:

- the docs-only queue is internally consistent, Child A is ready for activation,
  and no implementation or downstream lifecycle has been authorized

## Runbook Completion

This runbook is complete when Steps 1-4 produce and validate the ordered
documentation queue and handoff. Idea 732 itself remains open until all six
children execute in order and the umbrella-level final Markdown audit passes.
