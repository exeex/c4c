# RV64 Call Arg Local Frame Address Object Materialization Runbook

Status: Active
Source Idea: ideas/open/677_rv64_call_arg_local_frame_address_object_materialization.md

## Purpose

Resolve the RV64 object-route divergence for call arguments sourced from
`LocalFrameAddressMaterialization`.

## Goal

Either make object emission match the text-route direct
`addi a0, sp, offset` contract for local frame address call arguments, or
produce reviewed evidence that the current two-step object shape is the
intended contract.

## Core Rule

Do not treat byte-expectation churn, test-name matching, or baseline accounting
as progress. The route must be driven by the
`LocalFrameAddressMaterialization` semantic contract.

## Read First

- `ideas/open/677_rv64_call_arg_local_frame_address_object_materialization.md`
- `build/agent_state/675_step1_candidate_delta/summary.md`
- `build/agent_state/648_post656_call_evidence/summary.md`
- `test_baseline.log`
- `test_baseline.new.log`

## Current Targets

- Focus row:
  `backend_cli_riscv64_call_arg_local_frame_address_materialization`
- Owning contract:
  RV64 object-route consumption of `LocalFrameAddressMaterialization`
- Known paired-residual context:
  `test_baseline.new.log` remains unaccepted while row 159 is unresolved.

## Non-Goals

- Do not change the text-route contract unless fresh evidence proves it is
  wrong.
- Do not repair or reclassify the pointer/global-local publication row; idea
  676 closed that route through runtime proof and a positive object contract.
- Do not accept `test_baseline.new.log`.
- Do not edit unsupported markers, allowlists, timeouts, runtime policy, or
  baseline accounting.
- Do not key implementation to the filename, expected byte string, or test
  identity of the focus row.

## Working Model

Idea 675 identified this row as a remaining RV64 object-route divergence.
Prior evidence from idea 648 identified
`arg.source_selection=local_frame_address_materialization`. The text route
materializes the local frame address directly into the ABI argument register,
while the object route currently materializes through a saved register and then
copies to `a0`. This runbook must determine whether that object shape is a bug
in consuming the source-selection contract or an acceptable object contract
that needs documented positive proof.

## Execution Rules

- Preserve the prepared/source-selection distinction between local frame
  address materialization and generic register publication.
- Prefer semantic lowering or contract clarification over expectation rewrites.
- If implementation changes are needed, make the smallest general object-route
  repair that follows `LocalFrameAddressMaterialization` data rather than test
  identity.
- Focused proof must cover the target row and at least one nearby
  same-feature object-route case or negative boundary when available.
- Compare baseline rows by stable test name whenever referencing broad logs.

## Steps

### Step 1: Reconfirm The Object/Text Divergence

Goal: Establish the current object route and text route shapes from fresh
commands before editing code.

Actions:

- Inspect the focus row's current object-byte failure and expected contract.
- Re-read the idea 648 evidence for
  `arg.source_selection=local_frame_address_materialization`.
- Capture the text-route direct materialization shape and the object-route
  two-step shape with enough command output to identify the first divergent
  lowering boundary.
- Record findings under a focused `build/agent_state/677_*` directory if new
  proof artifacts are created.

Completion Check:

- The runbook has fresh evidence naming the exact object-route boundary that
  consumes `LocalFrameAddressMaterialization` differently from the text route,
  or a concrete reason the divergence is only contractual.

### Step 2: Repair Or Prove The Object Contract

Goal: Resolve whether direct ABI-register materialization is required for RV64
  object emission.

Actions:

- If direct `addi a0, sp, offset` is the intended contract, implement the
  smallest general object-route repair at the semantic consumption point.
- If the current two-step shape is valid, document the semantic or ABI evidence
  proving it should be accepted as the object contract.
- Do not weaken byte contracts or rewrite expectations as a substitute for
  implementation or reviewed contract evidence.
- Keep changes scoped to local frame address call-argument materialization.

Completion Check:

- The route has either a general implementation repair or a reviewed contract
  decision, both traceable to `LocalFrameAddressMaterialization` semantics.

### Step 3: Prove Focused And Nearby Coverage

Goal: Leave the supervisor with acceptance-quality evidence for the 677 route.

Actions:

- Run `cmake --build --preset default` when code or tests changed.
- Run the delegated focused proof for
  `backend_cli_riscv64_call_arg_local_frame_address_materialization`.
- Include at least one nearby same-feature object-route case or negative
  boundary when available.
- Update `todo.md` with proof results, remaining blockers, and whether
  baseline acceptance is still deferred.

Completion Check:

- The focus row and selected nearby coverage are green or the remaining
  blocker is precisely documented, and `test_baseline.new.log` remains
  unaccepted unless the supervisor delegates baseline acceptance after all
  paired residual rows are settled.
