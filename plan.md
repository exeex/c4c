# 118 AArch64 Calls Deferred Cluster Post-Contract Audit Runbook

Status: Active
Source Idea: ideas/open/118_aarch64_calls_deferred_cluster_post_contract_audit.md

## Purpose

Audit the calls-side clusters that idea 92 deferred, now that ideas 114, 116,
and 117 have added newer prepared-contract evidence. Produce bounded follow-up
ideas only when the current evidence supports a concrete owner boundary.

## Goal

Classify each deferred `src/backend/mir/aarch64/codegen/calls.cpp` cluster by
current ownership disposition and create focused `ideas/open/` follow-ups for
actionable, non-duplicative work.

## Core Rule

This route is analysis-only. Do not edit implementation files, test files,
build metadata, or `src/backend/mir/aarch64/codegen/calls.cpp`.

## Read First

- `ideas/open/118_aarch64_calls_deferred_cluster_post_contract_audit.md`
- `ideas/closed/92_aarch64_calls_owner_subresponsibility_audit.md`
- `ideas/closed/93_aarch64_calls_stack_frame_slot_operand_owner.md`
- `ideas/closed/94_aarch64_calls_f128_carrier_operand_owner.md`
- `ideas/closed/95_aarch64_calls_immediate_scalar_argument_publication_owner.md`
- `ideas/closed/114_prepared_outgoing_stack_argument_area_contract.md`
- `ideas/closed/116_aarch64_dispatch_prepared_producer_contract_surface.md`
- `ideas/closed/117_aarch64_comparison_fused_compare_publication_contract.md`

## Current Targets

Map these idea-92 deferred clusters against current code and closed-route
evidence:

- before-call move bundle lowering
- after-call result/value lowering
- preservation and republication lowering
- scalar producer dispatch bridge
- result recording and late publication
- ordinary call-boundary move/binding record construction where it directly
  couples to the deferred clusters

## Non-Goals

- Do not directly modify `calls.cpp`.
- Do not reopen completed ideas 93, 94, or 95.
- Do not rework outgoing stack area authority covered by idea 114.
- Do not reopen dispatch prepared producer, current-block publication, or
  comparison publication contracts covered by ideas 116 and 117.
- Do not propose one monolithic `calls.cpp` shrink route.
- Do not move AArch64-specific ABI spelling, scratch-register policy, or
  machine-record emission into shared BIR/prealloc.
- Do not touch x86 or RISC-V codegen implementation.

## Working Model

Use these dispositions consistently:

- `ready-local-owner`: extraction can stay AArch64-local and consume existing
  prepared facts.
- `move-forward-needed`: the cluster still reconstructs target-neutral facts
  that should move into shared BIR/prealloc first.
- `keep-in-calls`: the cluster is fundamentally call instruction or ABI record
  emission.
- `contract-needed`: focused tests, dumps, or route visibility are needed
  before movement is reviewable.
- `no-new-idea`: the cluster is already covered by a closed route or should
  intentionally remain local.

## Execution Rules

- Treat line-count reduction as irrelevant unless a concrete owner boundary is
  also identified.
- Cite the prepared/shared facts that support each classification, especially
  facts from ideas 114, 116, and 117.
- Follow-up ideas must name likely files, owner boundary, proof route, and
  concrete testcase-overfit reject signals.
- If a cluster is not actionable, record a clear `no-new-idea` disposition
  instead of manufacturing work.
- Keep routine findings in `todo.md` while executing; reserve source-idea edits
  for final closure or genuinely durable source-intent correction.

## Steps

### Step 1: Collect Closed-Route Evidence

Goal: Extract the durable closure facts relevant to the deferred calls
clusters.

Primary targets:

- `ideas/closed/92_aarch64_calls_owner_subresponsibility_audit.md`
- `ideas/closed/93_aarch64_calls_stack_frame_slot_operand_owner.md`
- `ideas/closed/94_aarch64_calls_f128_carrier_operand_owner.md`
- `ideas/closed/95_aarch64_calls_immediate_scalar_argument_publication_owner.md`
- `ideas/closed/114_prepared_outgoing_stack_argument_area_contract.md`
- `ideas/closed/116_aarch64_dispatch_prepared_producer_contract_surface.md`
- `ideas/closed/117_aarch64_comparison_fused_compare_publication_contract.md`

Actions:

- Inspect the closure notes and reject signals for ideas 92, 93, 94, 95, 114,
  116, and 117.
- Capture which calls clusters were deferred by idea 92 and which completed
  routes must not be reopened.
- Capture the specific prepared/shared facts from ideas 114, 116, and 117 that
  may now be consumed by calls-side follow-up owners.

Completion check:

- `todo.md` contains a concise evidence inventory keyed by closed idea and
  cluster, with explicit duplicate-work guardrails for ideas 93, 94, 95, 114,
  116, and 117.

### Step 2: Build The Current Calls Cluster Map

Goal: Map the deferred clusters to current function-level ownership in
`calls.cpp` without modifying code.

Primary target:

- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Inspect function names and nearby helper groupings for each deferred cluster.
- Identify where each cluster consumes prepared facts, where it mutates
  dispatch/call-boundary state, and where it emits AArch64-specific records.
- Include ordinary call-boundary move/binding record construction only where it
  directly couples to a deferred cluster.

Completion check:

- `todo.md` has a function-level map from each target cluster to current
  helper/function names, consumed facts, and retained AArch64-local emission
  responsibilities.

### Step 3: Classify Cluster Dispositions

Goal: Decide the current owner disposition for each deferred cluster using the
defined disposition labels.

Actions:

- Classify every target cluster as `ready-local-owner`,
  `move-forward-needed`, `keep-in-calls`, `contract-needed`, or `no-new-idea`.
- For every `ready-local-owner` classification, name the prepared facts that
  make a local owner boundary safe.
- For every `move-forward-needed` classification, name the target-neutral fact
  or query that should move before AArch64 contraction.
- For every `contract-needed` classification, name the missing dump, route
  test, or focused visibility that blocks safe movement.

Completion check:

- `todo.md` contains a disposition table covering every target cluster, with
  current evidence and reject-risk notes.

### Step 4: Draft Bounded Follow-Up Ideas

Goal: Create follow-up `ideas/open/` files only for actionable,
non-duplicative slices.

Primary targets:

- New files under `ideas/open/`, if justified by Step 3.

Actions:

- Draft one follow-up idea per bounded actionable owner or contract slice.
- Include likely files, owner boundary, proof route, acceptance criteria, and
  concrete reviewer reject signals in each new idea.
- Add explicit `no-new-idea` rationale in `todo.md` for clusters that remain
  local, blocked, or already covered.

Completion check:

- Every actionable cluster has a bounded follow-up idea, and every
  non-actionable cluster has an explicit `no-new-idea` rationale.

### Step 5: Closure Readiness Package

Goal: Prepare the audit for supervisor review and closure without code
validation.

Actions:

- Summarize the final cluster-to-disposition map.
- List generated follow-up ideas and their intended proof routes.
- Confirm no implementation, test, or build metadata files changed.
- Confirm no generated idea duplicates ideas 93, 94, 95, 114, 116, or 117.

Completion check:

- `todo.md` contains the final audit summary, changed-file list, and explicit
  lifecycle close recommendation for the supervisor.
