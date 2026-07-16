# LIR Nominal Type Family Architecture Umbrella Runbook

Status: Active
Source Idea: ideas/open/837_lir_nominal_type_family_architecture.md
Activated from: user-priority switch from parked idea 836 Step 1

## Purpose

Classify the responsibilities packed into universal `LirTypeRef`, decide exact
nominal family boundaries, and generate an evidence-backed ordered successor
queue without implementing type-family code inside this umbrella.

## Goal

Produce the architecture handoff documents and one-first-owner successor ideas,
with canonical module-owned aggregate identity first and terminal universal
`LirTypeRef` deletion last.

## Core Rule

837 is classification, architecture routing, and successor generation only.
Do not edit implementation or tests, generate successors before evidence and
boundary decisions, treat classification as capability, or erase parked owner
return obligations.

## Read First

- `ideas/open/837_lir_nominal_type_family_architecture.md`
- `ideas/open/836_lir_remaining_aggregate_owner_rejection_decomposition_blocker.md`
- `ideas/open/812_lir_string_authority_remaining_routes_umbrella.md`
- `ideas/open/813_lir_string_semantic_authority_completion_umbrella.md`
- the exact closed/open evidence inventory named by 837
- historical
  `origin/new_bir:ideas/open/746_bir_node_kind_centric_storage_pass_contract.md`

## Current Targets

- `docs/lir_nominal_type_family_architecture/`
- current `LirTypeRef` schema, producer, verifier, dispatch, printer, and
  receiver callsites as read-only evidence
- ordered new source ideas under `ideas/open/` only at Step 4

## Non-Goals

- Type-family implementation, schema/store/union code, producer migration,
  verifier/printer edits, tests, broad proof, or universal-model deletion.
- Activating generated successors or resuming/closing 836 inside this runbook.
- Revising 812/813/821/822 or unrelated ideas.
- Text parsing, every-string-is-debt classification, universal ID façades,
  giant optional bags, RTTI/vtable designs, or big-bang routes.

## Working Model

- Scalar, vector, aggregate, and function signature are nominal families.
- Only proven polymorphic first-class value boundaries receive bounded tagged
  unions with explicit enum kinds and custom checked access.
- Aggregate identity/layout/owner convergence is the first implementation
  successor because 832-836 expose it as the recurring wall.
- Rendering is one-way; old runtime-text and string APIs receive explicit
  deletion owners and remain until their consumers migrate.
- Parked 836 retains exact Step 1 resumption and parent 831 Step 4 return state.

## Execution Rules

- Record one evidence revision across all handoff documents.
- Distinguish accepted bounded capability from family-wide completion.
- Reuse existing open owners as dependencies; do not duplicate them.
- One first owning layer and coherent semantic contract per generated idea.
- Keep successor routes staged and buildable; name old responsibilities,
  accepted capability, adapters, proof, and deletion conditions.
- Apply execution updates to `todo.md` first; change this runbook only for a
  genuine route correction.

## Ordered Steps

### Step 1 - Establish the evidence baseline and historical root-model map

Goal: create one current evidence baseline and map each required historical
blocker/closure to the overloaded `LirTypeRef` responsibility it exposed.

Actions:

- Inspect current code surfaces and all exact closure/resumption evidence named
  by 837.
- Create `evidence_baseline.md` and
  `historical_blocker_root_model_map.md` under the handoff directory.
- Distinguish the HIR aggregate model that already exists (`HirStructDef`,
  aggregate field/layout facts, and the module owner index) from the missing
  occurrence-to-definition contract: every aggregate-bearing `QualType` must
  resolve through a stable reference to its canonical module-owned aggregate
  definition rather than through a reconstructed lookup key.
- Record the current twelve residual cases as three separate handoff-contract
  evidence groups, without claiming repair: incomplete `structured owner key`
  cases (50, 52, 510, 661, 688, 692, and 3038), present-but-unmatched
  `matching module owner` cases (201, 202, 217, and 1529), and the legitimate
  no-owner rendered-compatibility case (46).
- Record accepted capabilities, bounded exclusions, current open dependencies,
  and the parked 836 return obligation.
- Do not create successor ideas or edit implementation/tests.

Completion check: both documents cite one tree revision and every required
historical wall has a root responsibility, preservation rule, and owner status;
the HIR occurrence-to-canonical-definition seam and all three residual evidence
groups are explicit rather than being collapsed into a generic LIR failure.

### Step 2 - Build the responsibility matrix and nominal boundary decisions

Goal: exhaustively classify current responsibilities and decide nominal family
and bounded-union boundaries.

Actions:

- Create `current_lir_type_ref_responsibility_matrix.md` with exact fields,
  callsites, producers, consumers, recursive children, text precedence,
  existing owners, future first owners, adapters, and deletion conditions.
- Create `nominal_family_boundary_decisions.md` for scalar, vector, canonical
  module-owned aggregate, function signature, and exact polymorphic unions.
- Require the aggregate boundary and matrix to define a canonical typed
  ref/store graph: type occurrences refer to one module-owned definition and
  nested aggregates retain recursive typed child refs. Normalize ownership and
  representation during HIR-to-LIR lowering; do not flatten away field
  hierarchy, nested aggregate identity, or recursive semantic structure.
- Explicitly assess a stable `HirAggregateId`/`HirAggregateRef` as the HIR
  occurrence-to-definition carrier. If a different design is selected, record
  evidence that it provides the same stable, unique, module-owned reference
  contract without reconstructing identity from tags, rendered text, or parser
  pointers.
- Require compile-time separation targets, C++20 enum-kind traits/custom checked
  access, family overloads, one-way rendering, and fail-closed foreign/wrong-
  family behavior in future contracts.

Completion check: every current responsibility has one destination/disposition;
no generic runtime-text bucket, unrestricted union, or duplicate owner remains;
the chosen HIR carrier and recursive aggregate graph semantics are explicit.

### Step 3 - Define dependency order and one-first-owner successor scopes

Goal: turn the accepted matrix into executable, non-overlapping successor
contracts before assigning idea numbers.

Actions:

- Create `dependency_ordering.md`.
- Put canonical module-owned aggregate ref/store convergence first, consuming
  754/801/832-836 evidence without claiming 836 completion.
- Define that first aggregate successor around an explicit HIR canonical-store
  to LIR aggregate-store intern/mapping contract, including stable identity,
  mapping lifetime, and module ownership at the lowering boundary.
- Require its scope and proof contract to cover nested, local, template,
  anonymous, and typedef/alias aggregate occurrences; define registration
  ordering plus fail-closed foreign- and wrong-module behavior.
- Name deletion gates for parser `record_def` dependence, tag/rendered-text
  reconstruction or compatibility fallbacks, and duplicate owner/identity/
  layout metadata. Retain each adapter only until its named consumers have
  migrated and parity is evidenced.
- Order function-signature/call composition next where dependent; order vector,
  scalar, restricted-union, direct-construction, and overloaded-consumer routes
  from first-owner evidence.
- Keep universal `LirTypeRef`/`runtime_text`/string-conversion deletion terminal.
- Specify that accepted type-family capabilities precede an 812 refresh, then
  813 residual non-type string routing.

Completion check: every unresolved matrix row belongs to one coherent proposed
successor scope with dependencies, proof, deletion conditions, and no duplicate
open ownership; the first aggregate successor fully specifies HIR-to-LIR
mapping, aggregate-form coverage, module rejection behavior, and legacy
metadata/fallback retirement.

### Step 4 - Generate ordered successor ideas and handoff documents

Goal: create durable open source ideas for the accepted Step 3 scopes.

Actions:

- Create one source under `ideas/open/` per accepted first-owner scope, with
  mandatory reviewer reject signals and exact matrix-row mappings.
- Create `successor_queue.md` recording generated paths and ordering.
- Do not activate or implement any successor.
- Verify the aggregate successor is first and the universal-model deletion/
  convergence successor is terminal.

Completion check: all unresolved rows map exactly once to an existing owner or
generated open successor, and the queue is executable without mixed ownership.

### Step 5 - Record closure trace and exact downstream routing

Goal: make the umbrella result durable and request the correct lifecycle return.

Actions:

- Create `closure_trace.md` with evidence revision, documents, row counts,
  boundary decisions, generated ideas, reused owners, dependency order,
  deletion gates, parked 836 obligation, and 797 relationship.
- Record the intended next activation as the first generated canonical
  aggregate successor; do not activate it inside this step.
- Record 812 refresh after the type-family queue/capabilities, followed by 813
  residual non-type string routing.
- Send the exhausted runbook to plan-owner for explicit close/switch judgment;
  do not infer capability completion from docs or successor creation.

Completion check: closure trace is complete, no implementation capability is
claimed, and plan-owner has exact successor, 812/813, 797, and parked-836 routes.
