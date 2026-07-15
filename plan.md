# BIR NodeKind Schema and Pass Contract Runbook

Status: Active
Source Idea: ideas/open/746_bir_node_kind_centric_storage_pass_contract.md
Activated from: user-directed switch from idea 763; idea 763 is parked at its
unstarted Step 1 with a durable resumption record.

## Purpose

Define and begin enforcing a single C++ `NodeKind` traits/schema contract for
BIR storage and passes, including the small compile-time and runtime helper API
that replaces TableGen-style backend classification and behavior queries.

## Goal

Make `NodeKind` the durable authority for node classification, operand roles,
typing policy, effects, stage legality, and pass dispatch without introducing
an external `.td` language or broad backend rewrite.

## Core Rule

Keep one simple data-oriented node graph. Payloads carry parameters, operands
are input uses, and repeated semantic classification routes through the
`NodeKind` schema helpers; every pass handles its accepted vocabulary
explicitly and fails closed on unhandled kinds.

## Read First

- `ideas/open/746_bir_node_kind_centric_storage_pass_contract.md`
- `ideas/closed/735_bir_phase_a_import_raw_document_convergence.md`
- `ideas/closed/736_bir_phase_b_canonical_document_convergence.md`
- `src/backend/bir/core/README.md`
- `src/backend/bir/core/ir.hpp`

## Scope

- Revise the BIR core documentation to state the durable node storage,
  vocabulary-transition, and pass-dispatch contract.
- Define one C++ `NodeKind` schema/traits layer and a small helper API for both
  compile-time and runtime queries.
- Make only minimal core IR declaration changes needed to establish that
  contract and migrate nearby duplicated classification where justified.
- Preserve current behavior while distinguishing bootstrap compatibility
  fields from the durable architecture.

## Non-Goals

- Do not edit or reopen closed ideas 735 or 736.
- Do not claim B1-B7 implementation completeness from documentation or schema
  scaffolding.
- Do not rewrite unrelated pass frameworks, later BIR phases, MIR, register
  allocation, emission, frontend IR, or LIR/HIR semantics.
- Do not introduce TableGen, a `.td` file, a generated side language, a second
  MIR schema, or pass-visible template/type-list machinery.
- Do not replace the whole current container or resolve every future node kind
  in this runbook.

## Working Model

- `NodeId` is arena/view-provided stable identity and is not stored in `Node`.
- A node carries `NodeKind`, concrete type identity, input-only operands, and a
  kind-governed payload reference.
- `NodeKind` owns rules; concrete result type remains a node/value fact because
  some typing rules depend on operands, payload, or signatures.
- Phase-specific products stay in exact revision-keyed external maps.
- Multi-result forms must use an explicit normalized or compact representation;
  they must not remain hidden behind generic result vectors or operand slots.

## Execution Rules

- Keep each implementation packet small and behavior-preserving.
- Add nearby same-feature coverage for schema queries and fail-closed behavior;
  one named node kind is not sufficient proof.
- Prefer explicit `switch (node.kind)` dispatch plus reusable classification
  helpers over catch-all visitors or duplicated tables.
- If `ir.hpp` changes, require a fresh build before focused tests. Escalate to
  supervisor-selected broader proof at the final shared-code checkpoint.
- Keep unrelated user changes intact and do not weaken tests or verifier
  contracts.

## Ordered Steps

### Step 1 - Audit the current BIR core contract and query duplication

Goal: establish the exact delta between current storage/pass behavior and the
approved node-kind-centric contract.

Actions:

- compare the closed 735/736 contracts, core README, and `ir.hpp`;
- inventory existing opcode/kind definitions, payload/result conventions,
  classification tables, pass switches, and any `.td`/TableGen-like BIR use;
- identify the smallest schema/helper seam that can own stable kind facts;
- record whether current multi-result and vector fields are compatibility
  scaffolding or require an explicit bounded follow-up.

Completion check: the executor can name the schema insertion point, initial
query set, nearby consumers, and compatibility fields without expanding into
an unrelated storage rewrite.

### Step 2 - Establish the NodeKind schema and helper API

Goal: introduce the single C++ authority for stable node-kind properties.

Primary target: `src/backend/bir/core/ir.hpp` and the smallest justified nearby
core schema files.

Actions:

- define the initial traits/descriptor representation for the existing bounded
  `NodeKind` vocabulary;
- provide hidden implementation machinery plus pass-facing compile-time helpers
  such as `is_op_binary_v<K>` and runtime wrappers such as
  `is_op_binary(kind)`;
- cover a coherent initial property family including classification, arity or
  operand role, and at least one behavior policy such as effects or stage
  legality;
- make unsupported or unhandled queries fail closed rather than silently
  accepting an unknown kind.

Completion check: several representative semantic and non-semantic kinds are
queryable through one schema in compile-time and runtime contexts, with nearby
tests proving positive and negative cases and no external DSL.

### Step 3 - Align core storage and one bounded pass/query consumer

Goal: demonstrate that the schema is an operational pass contract, not only a
renamed classification table.

Actions:

- make the minimal declaration/comment changes needed to describe input-only
  operands, node/value result identity, concrete type lookup, and arena-owned
  `NodeId` identity;
- migrate one nearby repeated classification or bounded dispatch consumer to
  the helper API while preserving explicit `NodeKind` handling;
- retain bootstrap `results` or `std::vector` fields only with clear
  compatibility status; do not encode outputs as operand indices;
- add focused coverage for the migrated consumer and unhandled-kind rejection.

Completion check: one real consumer uses the shared schema, behavior remains
unchanged for handled kinds, and unknown kinds cannot pass silently.

### Step 4 - Publish the durable BIR phase and storage contract

Goal: make the accepted architecture explicit in the core README without
claiming downstream phase implementation.

Primary target: `src/backend/bir/core/README.md`.

Actions:

- document the flat node shape, stable arena identity, concrete type property,
  input-only operands, payload role, and external phase-product maps;
- document B1-B7 as target-independent vocabulary transition and B4 SSA as a
  canonical property rather than an A-phase prerequisite;
- document NodeKind-owned schema queries, helper API boundaries, fail-closed
  dispatch, and why BIR does not need a TableGen-style `.td` language;
- state the chosen single-result/multi-result contract or explicitly bound the
  remaining compact-result follow-up.

Completion check: documentation satisfies the source acceptance criteria,
accurately cites 735/736 as historical contracts, and clearly separates
current compatibility storage from the durable design.

### Step 5 - Prove the bounded contract

Goal: validate the documentation and shared core changes without testcase
overfit or scope drift.

Actions:

- review the documentation diff against every source acceptance and reject
  signal;
- run a fresh build or compile because shared BIR declarations changed;
- run focused core/schema and migrated-consumer tests with multiple node-kind
  families and negative/unhandled cases;
- obtain the supervisor-selected broader/full regression checkpoint for shared
  backend code.

Completion check: build and focused proof pass, documentation matches actual
code, no `.td`/duplicate schema was introduced, and broader proof is accepted
by the supervisor.
