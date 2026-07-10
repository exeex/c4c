# Stack-Carried Pointer Source Publication Materialization Runbook

Status: Active
Source Idea: ideas/open/653_stack_carried_pointer_source_publication_materialization.md

## Purpose

Repair the remaining stack-carried pointer source publication or
materialization boundary exposed after RV64 fused pointer branch terminator
lowering advanced.

## Goal

Make the `%t6` path in `src/20140828-1.c` and the `%t23` path in
`src/loop-2e.c` either consume explicitly fresh, materialized stack-carried
pointer sources or fail closed with a precise producer-authority diagnostic.

## Core Rule

Pointer source freshness and materialization must come from explicit semantic
producer facts upstream and explicit prepared facts at the consumer point. Do
not infer them from stack offsets, final assembly shape, source spelling, local
names, diagnostics, testcase identity, runtime outcomes, expectation changes,
unsupported markers, or pass/fail accounting.

## Read First

- `ideas/open/653_stack_carried_pointer_source_publication_materialization.md`
- `ideas/closed/645_rv64_branch_residual_terminator_fragment_lowering.md`
- `ideas/closed/636_prepared_branch_stack_source_freshness_publication.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`

## Current Scope

- Representative rows: `tests/c/external/gcc_torture/src/20140828-1.c` and
  `tests/c/external/gcc_torture/src/loop-2e.c`.
- Target values: `%t6` in `src/20140828-1.c` and `%t23` in `src/loop-2e.c`.
- Owned boundary: the explicit producer-to-consumer authority chain needed for
  prepared/RV64 stack-carried pointer source publication and materialization
  after terminator admission is no longer the first owner.
- Current first missing owner: the `src/loop-2e.c` `%t23` compare operand
  still needs an explicit semantic pointer producer and prepared
  `source_selection` chain before RV64 object emission can consume it.
- Parked downstream evidence: `src/20140828-1.c` `%t6` now has explicit
  semantic/prepared/RV64 branch source publication and object emission exits 0;
  its qemu runtime abort is downstream of this idea's pointer-source authority
  boundary and should not be pursued under Step 4A unless new evidence shows a
  source-publication regression.

## Non-Goals

- Do not reopen RV64 terminator-fragment admission closed by idea 645.
- Do not republish generic branch stack-source freshness already owned by prior
  branch freshness ideas unless refreshed evidence proves a distinct producer
  gap.
- Do not take direct-global stack-backed pointer branch operands from idea 654.
- Do not change ABI policy, runtime policy, expectations, unsupported markers,
  allowlists, timeouts, or pass/fail accounting.
- Do not special-case `src/20140828-1.c`, `src/loop-2e.c`, `%t6`, or `%t23`.

## Working Model

Idea 645 proved the fused pointer branch terminator shape can lower when the
condition and exactly one compared pointer operand have explicit branch
stack-load authority. Step 3 packets added semantic compare-operand producer
publication plus prepared/RV64 producer and consumer authority for explicit
stack-carried pointer source selections. Step 4 evidence shows the
`src/20140828-1.c` `%t6` branch source is now explicit, fresh, materialized,
and accepted by RV64 object emission, while its remaining runtime abort belongs
to a downstream callee/result or frame-slot value owner. The active idea
continues with `src/loop-2e.c` `%t23`, where semantic BIR still reaches
`%t24 = bir.ne ptr %t20, %t23` without an explicit `%t23` producer and prepared
preservation remains stack-slot-only without `source_selection`.

## Execution Rules

- Keep routine packet progress and proof paths in `todo.md`.
- Start by refreshing object, prepared-BIR, disassembly, and runtime evidence
  for both representative values before implementation.
- Identify the producer, selected stack slot, source value, consumer branch,
  and publication or materialization point before changing code.
- Add support only when explicit semantic or prepared facts prove the pointer
  source identity, freshness, selected stack home, and materialization at the
  selected consumer point.
- Preserve fail-closed diagnostics for missing source publication, stale stack
  slots, ambiguous pointer sources, mismatched homes, and unrelated branch
  shapes.
- Do not reconstruct `%t6`-class pointer provenance in call preservation,
  value-home classification, or RV64 codegen from stack offsets, source spelling,
  final comparison shape, diagnostics, or testcase identity.
- If refreshed evidence belongs to the direct-global branch boundary, request
  lifecycle switch to idea 654 instead of broadening this plan.

## Steps

### Step 1: Refresh Stack-Carried Pointer Evidence

Goal: Confirm the current first owner for `%t6` in `src/20140828-1.c` and
`%t23` in `src/loop-2e.c`.

Actions:

- Capture prepared-BIR, object, disassembly, and runtime evidence for both
  representative rows.
- Record the selected stack slot, source value, consumer branch, and current
  pointer materialization or publication facts for each target value.
- Distinguish stale source, unmaterialized source, missing publication,
  mismatched home, and unrelated downstream runtime failures.

Completion check:

- `todo.md` records the refreshed evidence paths, the shared or separate first
  owners for `%t6` and `%t23`, and the next implementation or split boundary.

### Step 2: Locate The Publication And Materialization Boundary

Goal: Find the narrow producer or RV64 consumer surface that should own one
stack-carried pointer source rule.

Actions:

- Trace where the selected pointer source is written, carried through stack
  storage, reloaded, and consumed by the branch path.
- Identify which prepared fact should publish freshness, selected source
  identity, materialized value identity, and selected stack home at the
  consumer point.
- Choose one first implementation family only if the evidence proves a common
  semantic rule for at least one representative row.
- Define focused positive and fail-closed test shapes for complete, missing,
  stale, ambiguous, and mismatched pointer source publications.

Completion check:

- `todo.md` names the owned implementation surface, selected first family,
  focused proof target, and rejection behavior to preserve.

### Step 3A: Publish LIR-to-BIR Compare Pointer Sources

Goal: Give address-valued LIR compare operands a semantic pointer producer or
equivalent address-materialization publication before prepared/prealloc builds
value homes and call preservation.

Actions:

- Inspect `src/backend/bir/lir_to_bir/memory/coordinator.cpp` and
  `src/backend/bir/lir_to_bir/scalar.cpp::lower_scalar_compare_inst` to locate
  the narrow place where `LirCmpOp` address-valued operands are lowered through
  `lower_value`.
- Publish a named BIR producer or explicit address-materialization fact for
  local-frame pointer compare operands only when the LIR operand carries a
  structured local-slot base plus byte-offset source.
- Preserve scalar compare lowering for non-pointer operands and unrelated
  branch shapes.
- Add focused positive coverage for a `%t6`-class local-frame pointer compare
  operand that later needs stack-carried source selection.
- Add fail-closed coverage for missing, ambiguous, stale, non-local, or
  unsupported pointer source shapes.

Completion check:

- Fresh build plus focused LIR-to-BIR/BIR proof shows the `%t6`-class compare
  operand has an explicit local-frame pointer source producer or address
  materialization fact, without testcase-specific matching.

### Step 3B: Connect Prepared And RV64 Stack-Carried Pointer Authority

Goal: Carry the Step 3A pointer producer through prepared/prealloc preservation
and consume it in RV64 without broad branch or stack rewrites.

Actions:

- Verify prepared/prealloc value homes and call preservation attach
  `source_selection` for the real `%t6` stack-carried path from the Step 3A
  producer, not from stack-slot inference.
- Keep stale, missing, ambiguous, and mismatched pointer source states rejected
  with precise diagnostics.
- Preserve the explicit RV64 consumer path for prior stack-slot preservation
  and reject stack-slot-only pointer branch operands.
- Add or preserve focused positive and negative coverage for complete and
  incomplete pointer source publication.

Completion check:

- Fresh build plus focused proof shows prepared output for
  `src/20140828-1.c` carries `source_selection` for `%t6` value id `19` /
  slot `#16+stack8`, and RV64 either materializes that explicit source or fails
  closed on a precise missing-authority diagnostic.

### Step 4: Prove Representative Integration

Goal: Show the representative stack-carried pointer rows advance past the
stale or unmaterialized pointer source failure mode.

Actions:

- Rerun focused coverage plus the RV64 GCC torture backend route for
  `src/20140828-1.c` and `src/loop-2e.c`.
- Capture BIR, prepared-BIR, object, disassembly, and runtime evidence showing
  the consumed pointer source is explicit, fresh, and materialized at the
  selected branch.
- Record any distinct downstream owner if either representative advances but
  does not fully satisfy the source idea.

Completion check:

- `todo.md` records representative proof for both target values and any
  remaining downstream owner.

### Step 4A: Publish The `%t23` Compare Pointer Source Chain

Goal: Give the `src/loop-2e.c` `%t23` branch operand the same explicit
semantic-to-prepared stack-carried pointer source authority now proven for the
`%t6` path, without inferring from stack offsets or testcase shape.

Actions:

- Use the Step 4 evidence under
  `build/agent_state/653_step4_representative_integration/` as the starting
  boundary: `%t23` has branch stack freshness for value id `27` in
  `slot #46+stack336`, but no semantic `%t23` producer and no prepared
  `source_selection`.
- Inspect the `loop-2e.c` LIR-to-BIR compare operand path and identify why the
  Step 3A local-frame pointer publication rule did not create an explicit
  `%t23` producer.
- Extend the semantic producer rule only for structured stack-carried pointer
  compare operands with a proven local-frame source, selected stack home, and
  complete materialization facts.
- Carry the producer through prepared/prealloc preservation so `%t23` gains an
  explicit `source_selection`; keep stack-slot-only, stale, ambiguous, and
  mismatched states rejected with precise diagnostics.
- Prove RV64 object emission for `src/loop-2e.c` either materializes that
  explicit `%t23` source or fails closed on a precise missing-authority
  diagnostic.

Completion check:

- Fresh build plus focused proof shows `src/loop-2e.c` prepared output carries
  `source_selection` for `%t23` value id `27` / `slot #46+stack336`, and RV64
  object emission advances past `unsupported_terminator_fragment` for the
  selected branch without named-case matching.

### Step 5: Run Broader Validation And Close Or Park

Goal: Decide whether the source idea is complete after focused and
representative proof.

Actions:

- Run the supervisor-selected broader validation for the affected prepared,
  RV64 backend, and representative GCC torture scope after focused proof is
  green.
- If acceptance criteria are satisfied, request plan-owner close with
  regression-guard proof.
- If a distinct producer or downstream owner remains, record it in `todo.md`
  and request lifecycle split or park instead of broadening this idea.

Completion check:

- Lifecycle state either closes the source idea with passing guard proof or
  records a precise blocked or follow-up owner without expanding this plan.
