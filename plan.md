# 270 Phase F5 Memory Accesses Prepared-Only Same-Consumer Fail-Closed Proof

Status: Active
Source Idea: ideas/open/270_phase_f5_memory_accesses_prepared_only_same_consumer_fail_closed_proof.md

## Purpose

Prove one bounded prepared-only fail-closed surface for
`PreparedFunctionLookups::memory_accesses` without hiding, demoting, or
rewrapping that public lookup.

## Goal

Name one existing public `memory_accesses` consumer and prove that an
internally coherent prepared-only memory row is rejected as Route 3/Route 5
semantic agreement when the matching semantic authority is absent or
non-agreeing, while prepared compatibility output remains byte-stable.

## Core Rule

This is a same-consumer proof packet. Do not delete, privatize, wrap, hide, or
rename away `PreparedFunctionLookups::memory_accesses`, and do not weaken
existing helper, oracle, debug, printer, wrapper, exact-output, fallback, or
baseline expectations.

## Read First

- Source idea:
  `ideas/open/270_phase_f5_memory_accesses_prepared_only_same_consumer_fail_closed_proof.md`
- Prior context named by the source idea:
  `ideas/closed/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md`
- Relevant implementation and tests discovered during Step 1.

## Current Targets

- One existing public `memory_accesses` consumer reached by current x86 or
  riscv proof surfaces.
- Preferred route: selected x86 Route 3 `LoadLocal` source-memory agreement
  path, if supported fixtures can express the prepared-only row.
- Fallback route: nearest riscv source-memory consumer that can express the
  same prepared-only authority gap with stable compatibility output.
- Helper/oracle tests only when they support the selected target-consumer proof;
  helper-only proof is not sufficient.

## Non-Goals

- No memory lookup demotion, deletion, privatization, wrapping, or public API
  hiding.
- No broad `PreparedFunctionLookups`, `PreparedBirModule`, or draft 155
  retirement.
- No broad stale-publication, byte-offset drift, or cross-publication mismatch
  proof unless the selected prepared-only row naturally requires it.
- No movement of storage, addressing, source-home, ABI, layout, register,
  stack, wrapper, formatting, emission, instruction spelling, or exact
  target-output policy into BIR.
- No expectation weakening or unsupported-status downgrades.

## Working Model

- Prepared compatibility output may remain public and observable.
- Prepared-only memory data is not semantic agreement by itself.
- The selected consumer must fail closed at the same Route 3/Route 5 authority
  boundary it normally depends on.
- The proof must use supported fixtures. Hand-built stale prepared state is a
  blocker, not acceptable progress.

## Execution Rules

- Keep changes small and tied to one named consumer and one named row.
- Before code or test edits, document the selected consumer, row, missing or
  non-agreeing authority facts, public compatibility output, and fixture path
  in `todo.md`.
- If no supported same-consumer prepared-only row exists, stop implementation
  work and return a blocker map for a narrower fixture-support idea.
- Preserve prepared compatibility output and status strings byte-for-byte
  unless the supervisor explicitly accepts a changed contract.
- Any code-changing step needs matching before/after proof through canonical
  `test_before.log` and `test_after.log`.
- Treat testcase-shaped matching, named-case-only shortcuts, or expectation
  rewrites as route drift.

## Ordered Steps

### Step 1: Identify consumer, row, and authority gap

Goal: Find the exact same-consumer proof target before editing tests or
implementation.

Primary target: Current x86 or riscv `memory_accesses` consumer tests and
fixtures.

Actions:

- Inspect existing public `PreparedFunctionLookups::memory_accesses`
  consumers reached by x86 and riscv proof surfaces.
- Prefer the selected x86 Route 3 `LoadLocal` source-memory agreement path if
  it can express a prepared-only row through supported fixtures.
- If x86 cannot express the row without synthetic stale state, inspect the
  nearest riscv source-memory consumer path.
- Name the prepared lookup or publication row being treated as prepared-only.
- Name the missing or non-agreeing Route 3 memory/source fact.
- Name the missing or non-agreeing Route 5 publication/source owner when
  relevant.
- Record the current public compatibility output that must remain stable.
- Explain why the row is reachable through supported fixtures rather than
  hand-built stale prepared state.

Completion check:

- `todo.md` records the chosen consumer, row, authority gap, compatibility
  output, supported-fixture rationale, and the narrow proof command the
  supervisor should delegate next.
- If no supported row exists, `todo.md` records the blocker map and recommends
  a narrower fixture-support idea instead of implementation.

### Step 2: Add or tighten the same-consumer fail-closed proof

Goal: Prove the selected consumer rejects the prepared-only row as semantic
agreement without losing public prepared compatibility output.

Primary target: The narrowest backend/helper test family selected in Step 1.

Actions:

- Add or adjust the narrow same-consumer proof for the selected row.
- Keep helper/oracle, prepared status strings, fallback names, route-debug
  output, prepared-printer output, wrapper output, exact target output, and
  baseline behavior stable.
- Avoid helper-only proof unless the selected target consumer is also covered.
- If implementation is required, repair semantic authority checking rather
  than adding named-testcase matching.
- Keep the proof bounded to missing or non-agreeing Route 3/Route 5 authority
  for the selected row.

Completion check:

- The selected consumer fails closed for missing, prepared-only, unsupported,
  fallback, and policy-sensitive behavior tied to the row.
- Prepared compatibility output remains observable and byte-stable.
- Matching narrow proof runs before and after using canonical regression logs
  when code changes are involved.

### Step 3: Validate the selected proof surface

Goal: Demonstrate that the slice did not weaken adjacent prepared compatibility
or selected-backend behavior.

Primary target: Supervisor-selected proof subset for the chosen consumer.

Actions:

- Run the narrow proof command selected in Step 1 or delegated by the
  supervisor.
- If the proof touches helper/oracle compatibility rows, include the matching
  prepared lookup helper tests.
- If the proof touches backend route-debug or handoff-boundary behavior,
  include the matching backend test family.
- Use `test_before.log` and `test_after.log` for regression proof when code
  changes are present.

Completion check:

- Narrow proof passes.
- No expectation weakening or unsupported-status downgrade is present.
- Any broader validation requested by the supervisor has a passing log.

### Step 4: Record closure guidance

Goal: Leave clear next-step evidence for the PO-family blocker from idea 265.

Primary target: `todo.md` execution summary, with source-idea closure left to
the plan owner only when the supervisor delegates close.

Actions:

- State whether the proof reduces the PO-family blocker from idea 265.
- State whether later `memory_accesses` proof packets should continue with
  stale-publication, byte-offset drift, or cross-publication mismatch rows.
- State any fixture-support gap discovered during the proof.
- Do not close the source idea merely because this runbook is exhausted; close
  only if the source idea acceptance criteria are satisfied and the close gate
  passes.

Completion check:

- `todo.md` contains an executor summary of proof results, remaining risks, and
  recommended continuation or close decision.
- The supervisor can decide whether to request close, rewrite, or follow-up
  idea creation without re-reading the entire diff.
