# Plan: Pointer/Address Semantic Model Research

Status: Active
Source Idea: ideas/open/597_pointer_address_semantic_model_research.md

## Purpose

Define the pointer/address semantic model that prepared value freshness,
local-memory boundary checks, relocation materialization, target consumption,
and later Prepared MIR view work must consume.

Goal: produce concrete research documents under
`docs/pointer_address_semantic_model_research/` that separate semantic
pointer/address authority from verifier/support facts, target-consume facts,
route proofs, and diagnostic-only artifacts.

Core Rule: this is a research route, not an implementation route; do not
migrate consumers, rewrite expectations, change unsupported markers, or
reopen closed branch stack-source work.

## Read First

- `ideas/open/597_pointer_address_semantic_model_research.md`
- Downstream consumer context:
  - `ideas/open/591_prepared_mir_view_contract_research.md`
- Selected freshness authority baseline:
  - `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
  - `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
  - `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
  - `ideas/closed/590_branch_stack_load_freshness_contract.md`
- Narrow branch pointer stack-source evidence:
  - `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
  - `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
  - `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`
  - `ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md`

Read `ideas/closed/` only as historical evidence for the paths named by the
source idea. Only `ideas/open/` is the candidate-work inventory.

## Current Targets And Scope

- Create:
  - `docs/pointer_address_semantic_model_research/index.md`
  - `docs/pointer_address_semantic_model_research/01_pointer_address_family_inventory.md`
  - `docs/pointer_address_semantic_model_research/02_semantic_authority_and_fact_classes.md`
  - `docs/pointer_address_semantic_model_research/03_fail_closed_rules.md`
  - `docs/pointer_address_semantic_model_research/04_closed_evidence_and_mir_boundary.md`
  - `docs/pointer_address_semantic_model_research/05_followup_recommendations.md`
- Survey prepared, prealloc, MIR, and target surfaces that consume or publish
  pointer/address evidence.
- Recommend one to three later implementation ideas only when each has a
  single first owner, prerequisites, proof surface, and reviewer reject
  signals.
- Preserve idea 591 as a downstream consumer of this model, not the owner of
  unresolved pointer/address semantics.

## Non-Goals

- Do not edit implementation files, tests, expectations, unsupported markers,
  allowlists, runtime behavior, or harness behavior.
- Do not migrate RV64, AArch64, x86, shared-prealloc, prepared/prealloc, MIR,
  or other consumers.
- Do not treat stack-home completeness, local-memory layout, relocation
  materialization, target operand shape, or diagnostic dumps as semantic
  pointer/address authority.
- Do not fold target consumer migration, shared producer publication repair,
  and Prepared MIR view design into this research route.
- Do not claim global pointer/address semantic closure from the closed branch
  pointer stack-source queue.

## Working Model

The research must classify each surveyed pointer/address family into one or
more explicit fact roles:

- semantic authority required to decide whether a pointer/address value is
  current for a use
- verifier or support fact that can prove a route but cannot authorize use on
  its own
- target-consume fact needed by a backend after semantic authority is already
  established
- route proof used for diagnostics, review, or debugging
- diagnostic-only artifact that must not become semantic authority

The branch pointer stack-source closures are evidence for a narrow selected
freshness subset. They are not proof that pointer arithmetic, relocation,
local-memory, stack-home, local-array, global-address, or target-shape
semantics are globally solved.

## Execution Rules

- Keep durable research in `docs/pointer_address_semantic_model_research/`.
- Cite concrete code surfaces, closed ideas, current docs, or command output
  for every semantic-authority claim.
- Use searches and read-only compiler inspection as needed, but keep all file
  edits to research docs and `todo.md`.
- If a family has unclear ownership or proof surface, mark it deferred instead
  of inventing a broad implementation route.
- Any recommended follow-up must be split by first owning layer and include
  prerequisites, proof surface, and reviewer reject signals.
- Update `todo.md` with packet progress and proof; do not rewrite this
  runbook for routine packet completion.

## Steps

### Step 1: Confirm Evidence Inputs And Research Output Shape

Goal: establish the exact evidence set and documentation contract for the
research route.

Actions:

- Read the source idea and the downstream idea 591 boundary.
- Read the named closed freshness and branch pointer evidence.
- Inspect existing docs for related prepared fact, target ABI, and MIR
  boundary research that should be cited or distinguished.
- Create the research docs directory and initial file skeletons only after the
  required answer shape is clear.

Completion Check:

- `todo.md` identifies the evidence paths used, any missing evidence, and the
  final research file list.

### Step 2: Inventory Pointer/Address Families And Consumers

Goal: identify the prepared pointer/address families that need semantic
classification.

Actions:

- Survey prepared and prealloc surfaces for pointer arithmetic
  materialization, semantic relocation, local stack or frame-slot addressing,
  local-array address derivation, global address materialization, branch
  pointer operands, and target-local operand shape.
- Trace the MIR and target consumers that currently depend on those facts.
- Write `01_pointer_address_family_inventory.md` with concrete file and
  symbol references for each family.

Completion Check:

- Each surveyed family names its current producer or evidence source, current
  consumer, and whether the family is semantic, support, target-consume,
  route-proof, diagnostic-only, or unresolved pending later classification.

### Step 3: Classify Semantic Authority And Fact Roles

Goal: decide which facts authorize pointer/address freshness and which facts
  are only supporting evidence.

Actions:

- Write `02_semantic_authority_and_fact_classes.md`.
- For each surveyed family, state the semantic authority rule for when a
  pointer/address value is current for a specific use.
- Separate semantic authority from verifier/support facts, target-consume
  facts, route proofs, and diagnostic-only artifacts.
- Explicitly classify stack-home completeness, local-memory layout, target
  operand shape, and relocation materialization where the source idea calls
  them out.

Completion Check:

- Every inventoried family has an authority decision or an explicit deferred
  status with the missing first owner or proof surface.

### Step 4: Define Fail-Closed Rules

Goal: make invalid pointer/address evidence rejectable without target-local
or diagnostic inference.

Actions:

- Write `03_fail_closed_rules.md`.
- Define missing, ambiguous, stale, wrong-value, wrong-use, stack-home-only,
  local-layout-only, relocation-less, and target-shape-only failure modes.
- State the expected fail-closed behavior for each surveyed family.
- Identify any cases where current code appears to accept support or
  diagnostic facts as semantic authority.

Completion Check:

- The document gives concrete fail-closed rules that a later implementation
  idea could test without relying on one named testcase or one target operand
  shape.

### Step 5: Position Closed Evidence And The Prepared MIR Boundary

Goal: prevent closed narrow routes or future MIR views from being mistaken
for the semantic model.

Actions:

- Write `04_closed_evidence_and_mir_boundary.md`.
- Classify ideas 587 through 590 as the selected freshness authority baseline.
- Classify ideas 592, 593, 594, and 596 as narrow branch pointer stack-source
  evidence, not global pointer/address semantic closure.
- State what idea 591 may consume from this research and what it must still
  avoid inventing inside `PreparedMirView`.

Completion Check:

- The document clearly separates evidence, unresolved semantics, and
  downstream MIR view consumption responsibilities.

### Step 6: Finalize Recommendations And Index

Goal: produce a reviewable research package and a narrow implementation
handoff only where justified.

Actions:

- Write `05_followup_recommendations.md`.
- Recommend zero to three later implementation ideas, each with first owner,
  prerequisites, proof surface, and reviewer reject signals.
- Mark unclear families deferred instead of expanding this route.
- Write `index.md` linking all numbered files and summarizing the semantic
  model, fact classes, fail-closed rules, and follow-up/deferred outcomes.

Completion Check:

- The research package satisfies the source idea acceptance criteria, and the
  closure note can state surveyed families, semantic authorities, fact roles,
  evidence boundaries, recommended follow-ups, deferred families, and the
  impact on idea 591.

### Step 7: Validate Research-Only Scope

Goal: prove the active route stayed within documentation and lifecycle
boundaries.

Actions:

- Run `git diff --check`.
- Inspect `git diff --name-only HEAD` and confirm changes are limited to
  research docs, `plan.md`, and `todo.md` unless a later supervisor-approved
  lifecycle action explicitly adds another allowed path.
- If all source-idea acceptance criteria are met, ask plan-owner closure to
  evaluate whether the idea can close.

Completion Check:

- `todo.md` records validation commands and confirms no implementation, test,
  expectation, unsupported-marker, allowlist, runtime, harness, or unrelated
  lifecycle files changed.
