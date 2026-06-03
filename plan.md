# AArch64 Calls Deferred Move And Publication Authority Audit Runbook

Status: Active
Source Idea: ideas/open/96_aarch64_calls_deferred_move_publication_authority_audit.md

## Purpose

Audit the call-related authority clusters deferred from idea 92 and decide
whether any have narrow, proofable follow-up implementation boundaries.

## Goal

Trace prepared call facts, dispatch state, scalar publication, preservation,
and AArch64 emission paths before proposing any implementation follow-up.

## Core Rule

This plan is audit-only. Do not edit implementation files or claim capability
progress from helper renames, expectation rewrites, or line-count reduction.

## Read First

- `ideas/open/96_aarch64_calls_deferred_move_publication_authority_audit.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/dispatch*.cpp`
- `src/backend/mir/aarch64/codegen/dispatch*.hpp`
- relevant prepared call, publication, and prealloc sources under
  `src/backend/prealloc/`
- historical boundary references:
  - `ideas/closed/69_aarch64_call_publication_prepared_authority_cleanup.md`
  - `ideas/closed/84_aarch64_prepared_consumer_wrapper_contraction.md`
  - `ideas/closed/92_aarch64_calls_owner_subresponsibility_audit.md`
  - `ideas/closed/93_aarch64_calls_stack_frame_slot_operand_owner.md`
  - `ideas/closed/94_aarch64_calls_f128_carrier_operand_owner.md`
  - `ideas/closed/95_aarch64_calls_immediate_scalar_argument_publication_owner.md`

## Scope

- before-call move bundle lowering
- after-call move, return move, value move, and preservation lowering
- scalar producer dispatch bridge
- result recording and late publication
- follow-up idea creation only when the traced authority boundary is narrow and
  proofable

## Non-Goals

- Do not modify calls, dispatch, prealloc, or test expectation files during the
  audit.
- Do not move AArch64 ABI construction, scratch choice, call spelling,
  materialization spelling, or machine-record construction into shared
  BIR/prealloc.
- Do not reopen ideas 93-95 as local-owner routes.
- Do not create one broad `calls.cpp` shrink or rewrite initiative.
- Do not re-derive prepared call plans, move bundles, after-call facts,
  preservation facts, aggregate transport facts, or publication facts under a
  new local helper name.

## Working Model

Classify each traced cluster with one or more of:

- `target-local-calls-emission`
- `prepared-fact-consumption`
- `dispatch-state-mutation`
- `candidate-local-subowner`
- `candidate-shared-authority-gap`
- `needs-narrow-implementation-idea`
- `intentionally-retained`

Use `todo.md` for packet notes, traces, and proof results. Only create or
change source ideas when the audit has produced durable follow-up intent.

## Execution Rules

- Keep evidence tied to concrete functions, helpers, records, and source fact
  producers/consumers.
- For each cluster, trace both the upstream prepared fact authority and the
  downstream AArch64 side effect or dispatch-state mutation.
- Treat AArch64 materialization and scratch selection as target-local unless
  the trace proves a shared prepared-fact boundary.
- When proposing a follow-up idea, include focused proof expectations for the
  affected call forms.
- If implementation files are accidentally touched, stop and require fresh
  build or compile proof before accepting the slice.

## Steps

### Step 1: Establish Deferred Cluster Map

Goal: build the audit map from idea 92 and the current call/dispatch/prealloc
surfaces.

Primary targets:

- `ideas/closed/92_aarch64_calls_owner_subresponsibility_audit.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/dispatch*.cpp`
- `src/backend/prealloc/`

Actions:

- Inspect the deferred clusters named by idea 92 and this source idea.
- Identify the current functions/helpers involved in each cluster.
- Record the initial cluster map in `todo.md`, including likely prepared fact
  producers, dispatch consumers, and AArch64 emission sites.

Completion check:

- `todo.md` names the concrete entry points for every deferred cluster and
  identifies which cluster should be traced next.

### Step 2: Trace Before-Call Move Bundle Lowering

Goal: classify before-call move bundle lowering from prepared move-bundle facts
to AArch64 call-boundary records and emitted side effects.

Primary targets:

- calls lowering helpers that consume prepared move bundles
- related prepared call/prealloc sources
- AArch64 call-boundary record construction and emission sites

Actions:

- Trace where prepared before-call move facts are produced.
- Trace where calls lowering consumes those facts.
- Separate target-local emission spelling from prepared-fact consumption.
- Decide whether the cluster is retained, local-subowner material, or a narrow
  implementation-idea candidate.

Completion check:

- `todo.md` records the authority classification and either names a proofable
  follow-up boundary or says why the cluster is intentionally retained.

### Step 3: Trace After-Call, Return, Value, And Preservation Lowering

Goal: classify after-call result moves, return moves, value moves,
preservation, and republication through prepared facts and dispatch state.

Primary targets:

- calls after-call/result/preservation lowering helpers
- dispatch state mutation paths
- prepared result and preservation fact sources

Actions:

- Trace after-call and return/value move facts from preparation through calls
  lowering.
- Trace preservation and republication paths through dispatch state mutation.
- Identify which behavior is target-local calls emission and which depends on
  shared prepared or dispatch authority.

Completion check:

- `todo.md` records classification for after-call/result/return/value and
  preservation paths, plus any narrow follow-up idea boundary.

### Step 4: Trace Scalar Producer Dispatch Bridge

Goal: classify scalar producer publication across prepared call argument source
producers, scalar materialization, and AArch64 materialization/address emission.

Primary targets:

- dispatch scalar producer bridge helpers
- scalar publication producers
- AArch64 scalar materialization and address emission helpers used for calls

Actions:

- Trace prepared call argument source producers into dispatch scalar handling.
- Separate scalar publication authority from AArch64 materialization spelling.
- Check indirect call materialization interactions when relevant.
- Decide whether the bridge exposes a shared-authority gap or remains
  intentionally retained.

Completion check:

- `todo.md` records bridge ownership, materialization boundaries, and any
  focused proof needed for a future implementation idea.

### Step 5: Trace Result Recording And Late Publication

Goal: classify result recording and late publication, including call result
source registers, FPR result store retargeting, missing frame-slot call
arguments, and stack-preserved value publication.

Primary targets:

- calls result-recording helpers
- late publication helpers
- prepared publication/prealloc sources
- dispatch record mutation paths

Actions:

- Trace call result source register recording.
- Trace FPR result store retargeting and missing frame-slot call argument
  diagnostics.
- Trace stack-preserved value publication through the prepared fact and
  dispatch-state boundaries.
- Classify each subcluster and identify any proofable follow-up boundaries.

Completion check:

- `todo.md` records which result-recording and late-publication clusters are
  retained and which need narrow implementation ideas.

### Step 6: Synthesize Audit Result And Follow-Up Ideas

Goal: finish the audit with durable conclusions and create only narrow
follow-up ideas that the trace supports.

Primary targets:

- `todo.md`
- `ideas/open/` for any new follow-up idea files
- `ideas/open/96_aarch64_calls_deferred_move_publication_authority_audit.md`
  only if closure or durable source-intent notes are required

Actions:

- Summarize every deferred cluster classification.
- For clusters classified `needs-narrow-implementation-idea`, create focused
  source ideas with concrete proof expectations and reviewer reject signals.
- Explicitly name clusters that remain `intentionally-retained`.
- Prepare the close decision for this source idea, including generated
  follow-up idea names.

Completion check:

- All deferred clusters are classified, follow-up ideas exist only where the
  boundary is narrow and proofable, and the source idea is ready for lifecycle
  close review.
