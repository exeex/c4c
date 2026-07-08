# RV64 Branch Stack-Source Freshness Consumption Runbook

Status: Active
Source Idea: ideas/open/593_rv64_branch_stack_source_freshness_consumption.md

## Purpose

Migrate one narrow RV64 branch stack-source consume path to selected shared
freshness authority now that idea 592 has provided the producer-side contract.

Goal: make RV64 consume shared `BranchStackLoadSource` freshness for an exact
branch use instead of relying on stack homes, frame slots, aggregate lanes, or
target-local structural inference.

## Core Rule

Do not make RV64 branch stack-source emission succeed from local structural
evidence alone. A migrated route must require selected shared freshness for the
same prepared source value, `PreparedValueFreshnessUseKind::BranchStackLoadSource`,
and the exact branch block plus terminator position being emitted.

## Read First

- `ideas/open/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- The focused closure/proof notes for ideas 587, 588, 589, 590, and 592.
- Current RV64 MIR branch emission and stack-source consume code.
- Shared prepared/prealloc freshness lookup APIs and diagnostics used by RV64.

## Current Targets And Scope

- RV64 branch emission and stack-source consume paths that currently depend on
  structural stack homes, frame slots, typed stack-source facts, aggregate
  stack-source facts, or branch stack-load rows.
- Pointer `Lhs` / `Rhs` branch stack-load operands and any typed or
  aggregate-adjacent branch stack-source operands made publishable by idea 592.
- Focused RV64 proof that accepted routes consume selected shared authority and
  fail closed for missing, ambiguous, stale, wrong-value, wrong-use,
  future-point, and stack-home-only authority.

## Non-Goals

- Do not define or repair typed or aggregate producer publication; send that
  back to the 592 family if missing.
- Do not infer freshness in RV64 from a stack home, frame slot, aggregate lane,
  clobber-safety fact, register allocation fact, or operand shape.
- Do not migrate AArch64 or x86.
- Do not design the Prepared MIR view line from idea 591.
- Do not redesign control-flow lowering, branch instruction selection, ABI
  classification, register identity policy, or the freshness data model.
- Do not claim progress through expectation downgrades, unsupported-marker
  edits, allowlist edits, or runtime output changes.

## Working Model

- Shared prealloc is the authority for branch stack-load source freshness.
- RV64 is a consumer only. It may query selected freshness and report precise
  diagnostics, but it must not manufacture fallback freshness facts.
- A stack home, frame slot, aggregate lane, or clobber-safety fact can support
  layout and safety decisions, but none of those facts is freshness by itself.
- Missing producer authority is a producer-side blocker, not permission to add
  a target-local RV64 shortcut.

## Execution Rules

- Keep each implementation packet narrow enough to prove with a focused RV64
  command chosen by the supervisor.
- Before editing RV64 logic, identify the exact selected consumer path and the
  shared freshness query it should use.
- Preserve diagnostics that distinguish missing source freshness from missing
  stack home, missing layout, clobber failure, unsupported operand shape, and
  missing producer publication.
- Treat named-case matching or testcase-shaped branching as route drift.
- Keep existing focused coverage from ideas 587 through 592 green.
- Closure must inventory every remaining RV64 branch stack-source consumer
  family and either open the concrete 594 follow-up or prove no 594 is needed.

## Ordered Steps

### Step 1: Inspect Producer Contract And RV64 Consumers

Goal: choose the narrow RV64 consumer path to migrate using completed 592
producer evidence and current RV64 code.

Primary target: RV64 MIR branch/stack-source consume surfaces and shared
prepared/prealloc freshness lookup APIs.

Actions:

- Read the idea 592 closure note and focused proof to confirm which typed and
  aggregate `BranchStackSlot` freshness facts are published.
- Trace RV64 branch emission paths that consume branch stack-source operands,
  especially pointer `Lhs` / `Rhs` and typed or aggregate-adjacent stack-load
  operands left blocked by idea 590.
- Identify where RV64 currently uses stack homes, frame slots, aggregate lanes,
  clobber facts, or operand shape as a freshness proxy.
- Select exactly one narrow consumer path for the first migration packet.
- Record any missing producer fact as a blocker instead of patching it in RV64.

Completion check:

- `todo.md` names the selected RV64 consumer path, the shared freshness query
  it will use, and any producer-side blockers found before implementation.

### Step 2: Wire The Selected RV64 Consumer To Shared Freshness

Goal: make the selected RV64 path require selected shared freshness for the
exact branch stack-load source use.

Primary target: the RV64 branch emission or stack-source helper chosen in
Step 1.

Actions:

- Replace the local freshness inference point with a shared prepared/prealloc
  freshness lookup.
- Require the same prepared source value.
- Require `PreparedValueFreshnessUseKind::BranchStackLoadSource`.
- Require the producer proof kind accepted by ideas 592 and 590.
- Require the exact branch block and terminator position being emitted.
- Keep layout, clobber, and unsupported-shape checks separate from source
  freshness checks.

Completion check:

- The selected route cannot succeed without selected shared freshness for the
  exact branch use, and existing local structural facts no longer act as a
  fallback freshness authority.

### Step 3: Preserve Fail-Closed Diagnostics

Goal: make missing source freshness visible and distinct without weakening
  existing RV64 failure modes.

Primary target: RV64 diagnostics or prepared/RV64 dump status around the
selected consumer path.

Actions:

- Preserve or add diagnostics for missing source freshness.
- Keep missing source freshness distinct from missing stack home, missing
  layout, clobber failure, unsupported operand shape, and missing producer
  publication.
- Ensure stale, wrong-value, wrong-use, ambiguous, future-point, and
  stack-home-only authority fail closed with visible status.

Completion check:

- Focused negative cases expose distinct failure reasons or dump status without
  accepting stack-home-only or target-local evidence as freshness.

### Step 4: Add Focused RV64 Proof

Goal: prove the migrated path accepts shared authority and rejects nearby stale
or structural-only authority.

Primary target: focused RV64 tests, dumps, or proof fixtures selected by the
supervisor.

Actions:

- Add or update focused proof for at least one previously blocked RV64 branch
  stack-source case made possible by idea 592.
- Prove the accepted route consumes selected shared freshness.
- Prove missing, ambiguous, stale, wrong-value, wrong-use, future-point, and
  stack-home-only authority fail closed.
- Keep focused freshness coverage from ideas 587 through 592 green.

Completion check:

- The delegated proof command passes, and the proof shows capability through
  semantic shared-authority consumption rather than expectation rewrites or
  unsupported-marker changes.

### Step 5: Closure Inventory And 594 Handoff

Goal: close the 593 runbook only when the source idea completion questions are
answered and the next RV64 consume-side gap is identified.

Primary target: lifecycle closure notes and, if required, the existing
`ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md`
handoff.

Actions:

- Answer every completion question from the source idea.
- Inventory which RV64 consumer path was migrated.
- Inventory remaining RV64 branch stack-source shapes and why they remain
  unwired or intentionally deferred.
- State whether any selected RV64 path still relies on stack-home-only,
  frame-slot-only, aggregate-lane-only, clobber-only, or operand-shape-only
  evidence as freshness.
- State whether a missing shared producer fact must return to the 592 family.
- State whether RV64 branch stack-source behavior is stable enough for the 591
  Prepared MIR view line to treat it as a required backend input.
- Confirm the exact gap that 594 should take over next, or explain why no 594
  is needed.

Completion check:

- The source idea is ready for plan-owner close review, with closure answers,
  proof results, and 594 handoff status recorded at the correct lifecycle
  layer.
