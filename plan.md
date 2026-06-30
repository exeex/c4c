# RV64 Move-Bundle Residual Owner Audit Plan

Status: Active
Source Idea: ideas/open/460_rv64_move_bundle_residual_owner_audit.md

## Purpose

Identify the next exact owner of the remaining RV64 move-bundle failure after
explicit select-edge suppression consumption closed.

## Goal

Classify the current `20010329-1` `unsupported_move_bundle_target_shape` with
enough evidence to close, split, or activate the next precise source idea
without implementing broad generic move support.

## Core Rule

Do not implement or infer from raw shape in this audit. The output must be an
owner classification with evidence, not a generic lowering patch.

## Read First

- ideas/open/460_rv64_move_bundle_residual_owner_audit.md
- ideas/closed/459_rv64_select_edge_suppression_placement_consumer.md
- build/agent_state/459_step4_residual_disposition/disposition.md
- build/agent_state/459_step4_residual_disposition/probe_status.tsv
- build/agent_state/459_step4_residual_disposition/20010329-1.prepared.out
- build/agent_state/459_step4_residual_disposition/20010329-1.object.err
- build/agent_state/459_step4_residual_disposition/gdb_probe.out
- src/backend/mir/riscv/codegen/object_emission.cpp
- src/backend/prealloc/prepared_object_traversal.cpp

## Current Targets

- `20010329-1` prepared route passes.
- `20010329-1` RV64 object route fails with
  `unsupported_move_bundle_target_shape`.
- Step 4 debugger probe showed multiple move-bundle fragments before failure,
  but no exact optimized argument-local coordinate.
- Explicit `predecessor_edge_consumed_suppression` consumption is complete.
- Stale stack-load authority and generic register/stack move support remain
  out of scope.

## Non-Goals

- Implementing RV64 move-bundle lowering during the audit.
- Generic stack-to-register or register-to-register move support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Reopening ideas 456, 458, or 459.
- Pointer-value provenance, generic stack-home branch materialization, local or
  global store publication, expectation rewrites, unsupported-marker changes,
  allowlist edits, baseline churn, or pass/fail accounting changes.
- Touching `review/`, `test_before.log`, or `test_after.log`.

## Working Model

The broad RV64 diagnostic is no longer enough to choose a semantic packet.
This plan gathers only the evidence needed to identify the first remaining
move-bundle event and route it to the correct source idea.

## Execution Rules

- Start with read-only evidence classification.
- Use fresh probes under `build/agent_state/460_step1_move_bundle_residual_audit/`.
- Instrumentation may be read-only or local/transient, but implementation
  changes are not part of this audit packet.
- Keep stale stack-load authority, generic move support, and raw-shape
  inference rejected.
- Classification-only proof: `git diff --check`.

## Steps

### Step 1: Audit Remaining Move-Bundle Failure

Reproduce the prepared/object probes and locate the first unsupported
move-bundle event after idea 459. Record route command, exit status, object
diagnostic, candidate event coordinates, move-bundle rows, and ownership
classification. Completion means `todo.md` contains a residual-owner table and
states whether to close, split, or activate the next precise idea.

### Step 2: Residual Disposition And Close Readiness

Decide whether the audit is complete, whether a new implementation idea is
needed, or whether an existing open idea owns the residual. Completion means
close this audit with durable follow-up or keep it active only with an exact
remaining classification packet.
