# RV64 20000622-1 Foo Logical Select Runtime Abort Runbook

Status: Active
Source Idea: ideas/open/578_rv64_20000622_1_foo_logical_select_runtime_abort.md
Activated from: split follow-up to closed idea 577 evidence

## Purpose

Repair the remaining RV64 object-route runtime abort in the `foo` logical/select
family exposed after the 577 `baz` argument-source route was fixed.

## Goal

Classify the first bad `foo` logical/select owner, add focused non-filename
coverage for that shape, and repair the underlying RV64 prepared-value
publication semantics without regressing the fixed 577 `baz` path.

## Core Rule

Do not make `src/20000622-1.c`, `foo`, `%t13`, `%t24`, block names, value names,
or disassembly offsets part of the repair contract.

## Read First

- `ideas/open/578_rv64_20000622_1_foo_logical_select_runtime_abort.md`
- `build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/classification-summary.txt`
- `build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/dump-prepared-bir.txt`
- `build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/c4c.bin.disasm`
- `build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/clang.bin.disasm`

## Current Targets

- RV64 object-route scalar logical/select publication inside `foo`-style
  condition chains.
- Prepared select-chain materialization and carrier alias authority for binary
  and immediate select sources.
- Preservation of the 577 route where incoming `a2` is materialized through
  `ptrtoint`, preserved across `bar`, and passed as `foo` argument 0.

## Non-Goals

- Do not reopen the 577 `baz` formal pointer `ptrtoint` repair unless fresh
  evidence proves it regressed.
- Do not reopen same-module `CallInst` fallback work from 572.
- Do not do generic select or phi-select cleanup outside this logical-condition
  runtime family.
- Do not edit expectations, unsupported markers, allowlists, runtime comparison
  contracts, or gcc_torture runner behavior.
- Do not add filename-specific or emitted-name-specific handling.

## Working Model

The representative has advanced past the old `baz` argument source failure.
Current evidence points at later `foo` logical/select publication: prepared BIR
contains selects like `bir.select ne i64 %p.a, 12, i32 1, %t9` and
`bir.select ne i32 %t13, 0, i32 1, %t20`, with missing or unsupported carrier
alias authority for select inputs feeding the published results.

## Execution Rules

- Start each packet from artifacts, not assumptions from the old 577 failure.
- Preserve the current `baz` route and prove it when a packet can affect call
  argument or prepared-value publication.
- Add focused coverage for the semantic logical/select shape before or with the
  repair; coverage must not depend on the representative filename.
- Keep changes scoped to the classified first-bad owner. If evidence points to
  a separate initiative, stop and route that through lifecycle state instead of
  expanding this plan.
- Treat classification-only diffs, helper renames, expectation edits, and
  unsupported-marker changes as non-progress.
- Use the supervisor-delegated proof command for each packet and record proof in
  `todo.md`; broader validation is a supervisor decision.

## Ordered Steps

### Step 1: Rehydrate Evidence And Reproduce

Goal: establish the current failure surface and confirm it is still the
post-577 `foo` logical/select abort.

Primary Target: saved 577 artifacts and the current RV64 object-route
representative rerun.

Actions:

- Inspect the saved classification summary, prepared BIR, and both disassemblies
  listed in Read First.
- Reproduce the current `src/20000622-1.c` RV64 object-route abort using the
  supervisor-delegated command.
- Record whether the rerun still reaches the post-577 state where the `baz`
  argument source is correct and the later abort is in the `foo` condition path.

Completion Check:

- `todo.md` records the artifact paths, rerun command, result, and whether the
  first remaining bad area is still the `foo` logical/select family.

### Step 2: Classify The First Bad Owner

Goal: identify whether the first bad owner is select materialization, carrier
alias publication, logical short-circuit lowering, or a related prepared-value
publication rule.

Primary Target: RV64 prepared-value/select publication code identified by the
Step 1 artifact trail.

Actions:

- Trace the prepared-BIR select chain through RV64 lowering to object behavior.
- Compare c4c and clang behavior at the first point where the selected logical
  value becomes stale, inverted, missing, or published through the wrong carrier.
- Record concrete first-bad evidence using prepared-BIR, emitted object, runtime
  trace, or equivalent diagnostics.

Completion Check:

- `todo.md` names the first bad owner and includes enough artifact evidence for
  an executor or reviewer to distinguish the selected repair route from the old
  577 `baz` route and from 572 same-module call fallback work.

### Step 3: Add Focused Coverage

Goal: protect the repaired scalar logical/select publication shape without
overfitting the representative.

Primary Target: the narrowest existing backend or runtime coverage bucket that
can express the observed logical/select carrier-publication shape.

Actions:

- Add or extend focused coverage for the relevant scalar logical/select chain.
- Avoid representative filename, function-name, value-name, block-name, and
  disassembly-offset dependencies.
- Include an assertion or runtime behavior that fails for stale, inverted, or
  missing select publication.

Completion Check:

- The focused test fails before the semantic repair or is otherwise justified as
  covering the classified bad shape, and it passes after the repair.

### Step 4: Repair The General Publication Rule

Goal: make the RV64 object route publish the selected logical value through the
right prepared carrier for the classified shape.

Primary Target: the backend lowering/publication path proven by Step 2.

Actions:

- Implement the smallest semantic repair for the classified owner.
- Keep existing special cases stable but do not extend testcase-shaped matching.
- Preserve source, destination, and alias authority for select operands and
  materialized results according to the classified prepared-value model.
- Re-run the focused proof and the representative command delegated by the
  supervisor.

Completion Check:

- The focused coverage passes, the representative no longer aborts through the
  `foo(12, 1, 11)` failure path, and `todo.md` records the exact proof command
  and result.

### Step 5: Guard Adjacent Regressions

Goal: prove the repair did not regress the adjacent routes that made this idea
possible.

Primary Target: supervisor-selected 572 same-module call/result coverage and
577 `baz` ptrtoint materialization behavior.

Actions:

- Run the supervisor-selected guard subset for 572 behavior.
- Run the supervisor-selected guard subset for the 577 `baz` route.
- Escalate to broader validation if the supervisor determines the repair
  touched shared prepared-value, select, call, or publication machinery.

Completion Check:

- `todo.md` records green guard proof for the selected 572 and 577 subsets, or
  records the exact blocker preventing acceptance.

## Completion Criteria

- The first `foo` logical/select bad owner is recorded with artifacts.
- Focused non-filename coverage proves the repaired scalar logical/select
  publication shape.
- The representative RV64 object-route rerun no longer aborts because of stale,
  inverted, or missing select publication in `foo`.
- Existing 572 same-module call/result coverage and 577 `baz` ptrtoint
  materialization behavior remain intact.
