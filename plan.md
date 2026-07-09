# Aggregate Global-Object Materialization Policy Runbook

Status: Active
Source Idea: ideas/open/641_aggregate_global_object_materialization_policy.md

## Purpose

Turn the idea 641 aggregate global-object residuals into a narrow execution
route that distinguishes aggregate or byte-lane materialization from direct
scalar global-symbol local memory, aggregate stack-home policy, and large
selected pointer offsets.

## Goal

Define and repair the producer or RV64 consumer policy for one proven
aggregate global-object materialization family, or split the refreshed rows
into precise existing owners with evidence.

## Core Rule

RV64 may materialize an aggregate global-object access only from explicit
global object identity, aggregate lane, byte range, offset, extent, selected
destination, and memory-use authority. Do not infer those facts from final
assembly layout, source spelling, object names, or testcase identity.

## Read First

- `ideas/open/641_aggregate_global_object_materialization_policy.md`
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
- `ideas/open/633_aggregate_stack_home_local_memory_policy.md`
- `ideas/open/634_large_selected_pointer_offset_local_memory_policy.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`

## Current Targets

- Representative idea 631 Step 5 residual rows:
  - `src/complex-7.c`
  - `src/pr49073.c`
  - `src/pr60017.c`
  - `src/pr60822.c`
  - `src/pr88739.c`
- Nearby rows only when current diagnostics prove the same aggregate
  global-object or byte-storage materialization boundary.

## Non-Goals

- Do not reopen direct global-symbol local-memory support from idea 631.
- Do not implement aggregate/sret/byval stack-home local-memory policy owned
  by idea 633.
- Do not implement large selected pointer-offset policy owned by idea 634.
- Do not change prepared global value-location consumption owned by idea 621.
- Do not change ABI, runtime/library policy, expectations, unsupported
  markers, allowlists, timeouts, or accounting.

## Working Model

The first packet must refresh diagnostics before implementation. The route can
continue only if the rows expose a shared complete-authority shape. If the
evidence points at stack-home aggregate, large selected pointer offsets,
scalar direct globals, or runtime-only behavior, record that owner in
`todo.md` and stop rather than broadening this runbook.

## Execution Rules

- Keep source-idea intent stable; routine findings belong in `todo.md`.
- Treat testcase-shaped handling of the representative files or aggregate
  offsets as route drift.
- Preserve fail-closed diagnostics for missing lane identity, incomplete byte
  ranges, unsupported offsets, ambiguous destination authority, and mismatched
  aggregate extent.
- Every code-changing step needs fresh build proof plus a narrow backend or
  torture proof selected by the supervisor.
- Escalate to broader validation if a change touches shared local-memory,
  aggregate lane, byte-storage, or RV64 memory-use authority paths.

## Step 1: Refresh And Classify Aggregate Global Residuals

Goal: establish the current first owner for the representative rows before
choosing an implementation family.

Concrete actions:

- Reproduce diagnostics for the current target rows.
- Capture, per row, global object identity, aggregate lane, byte range,
  selected offset, width, extent, selected destination, and memory-use
  authority when present.
- Classify each row as aggregate global-object materialization, byte-storage
  lane materialization, aggregate stack-home policy, large selected pointer
  offset, scalar direct global-symbol local memory, runtime-only, or another
  precise owner.
- Record rows that lack complete authority separately from rows with a shared
  complete-authority shape.

Completion check:

- `todo.md` names the refreshed rows, their first owners, the exact missing or
  present authority facts, and one selected family for Step 2, or records why
  no valid family exists under idea 641.

## Step 2: Select One Shared Authority Family

Goal: narrow implementation to one semantic producer or RV64 consumer family
with complete evidence.

Concrete actions:

- Choose exactly one family from Step 1 evidence, such as aggregate
  global-object lane materialization or byte-storage global-object lane
  materialization.
- Identify whether the missing repair belongs in a prepared producer,
  prealloc fact, RV64 consumer, or diagnostic boundary.
- Define the negative states that must remain rejected.
- Leave rows outside the selected family fail-closed with owner notes in
  `todo.md`.

Completion check:

- The next executor packet has one owner, one positive authority shape, and
  explicit negative cases; no implementation packet spans multiple residual
  families.

## Step 3: Implement The Narrow Producer Or Consumer Repair

Goal: move one complete-authority aggregate materialization shape past the
current blocker without weakening unrelated local-memory policies.

Concrete actions:

- Add or adjust producer facts only when prepared evidence proves object
  identity, lane, byte range, offset, extent, destination, and memory-use
  authority.
- Add or adjust RV64 consumption only when the selected facts are explicit and
  unambiguous.
- Keep unsupported or diagnostic paths precise for missing, stale, ambiguous,
  or mismatched facts.
- Avoid filename, offset, source-spelling, and final-assembly matching.

Completion check:

- A narrow proof shows at least one selected aggregate materialization row
  advances past the old blocker, and nearby missing-authority cases still
  reject for precise reasons.

## Step 4: Prove Boundaries And Reclassify Spillover

Goal: verify that idea 641 did not absorb owners from adjacent ideas.

Concrete actions:

- Run supervisor-selected narrow proof for the changed code.
- Add negative proof for scalar direct global-symbol rows, stack-home
  aggregate rows, large-offset rows owned by idea 634, and runtime-only
  failures where applicable.
- Record any rows that still belong to idea 633, idea 634, idea 631, runtime
  ownership, or a new follow-up idea.

Completion check:

- Proof logs show the selected family is repaired or precisely blocked, while
  adjacent families remain outside this policy.

## Step 5: Final Lifecycle Review

Goal: decide whether idea 641 is complete, needs a replacement runbook, or
should split remaining work.

Concrete actions:

- Compare implementation and proof against the source idea acceptance
  criteria.
- Confirm no progress claim depends on expectations, unsupported markers,
  allowlists, timeouts, runtime policy, or accounting.
- Ask the plan owner to close, rewrite, deactivate, or split only after the
  source-idea completion state is clear.

Completion check:

- The supervisor has enough evidence to request close, continue with a new
  runbook, or create a separate open idea for remaining owner-specific work.
