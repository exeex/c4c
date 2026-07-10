# RV64 Pointer Global Local Publication Runtime Contract Runbook

Status: Active
Source Idea: ideas/open/676_rv64_pointer_global_local_publication_runtime_contract.md

## Purpose

Settle the now-succeeding RV64 pointer/global-local publication expected-fail
row by proving object semantics before any test-contract or baseline route.

## Goal

Determine whether the emitted RV64 object is semantically valid; then route the
row as stale expected-failure/test-contract work or identify the fresh
implementation owner from runtime evidence.

## Core Rule

Do not claim compiler progress, change expectations, or accept
`test_baseline.new.log` until runtime/semantic proof explains the unexpected
object success for this row.

## Read First

- `ideas/open/676_rv64_pointer_global_local_publication_runtime_contract.md`
- `ideas/open/675_post_wave_residual_baseline_failures.md`
- `build/agent_state/675_step1_candidate_delta/summary.md`
- `test_baseline.log`
- `test_baseline.new.log`
- `ideas/closed/664_riscv_object_emission_internal_probe.md`

## Current Targets

- Focus row:
  `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
- Generated object:
  `build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o`
- Step 1 evidence:
  `build/agent_state/675_step1_candidate_delta/summary.md`

## Non-Goals

- Do not edit expectations, unsupported markers, allowlists, timeouts, runtime
  policy, or baseline accounting in this runbook unless the supervisor
  explicitly delegates that contract route after proof exists.
- Do not repair
  `backend_cli_riscv64_call_arg_local_frame_address_materialization`; that row
  is split to idea 677.
- Do not reopen idea 664 or older pointer/global-local owner notes without
  fresh runtime evidence contradicting their closure.
- Do not accept `test_baseline.new.log`.

## Working Model

Step 1 of idea 675 proved that the old RV64 object-route local-memory admission
owner moved. The row now fails because the expected-failure wrapper reports
unexpected success. The object disassembly shows direct global publication into
stack slots and a live reload. The next step is semantic proof: valid object
behavior means stale expected-failure/test-contract ownership; invalid object
behavior means a new implementation owner must be identified from runtime
evidence.

## Execution Rules

- Preserve `build/agent_state/675_step1_candidate_delta/summary.md` as the
  source evidence for this split.
- Keep any new proof artifacts under a focused `build/agent_state/676_*`
  directory.
- Prefer runtime/semantic observation over disassembly-only inference.
- If code changes become necessary, first record the exact semantic mismatch
  and then run `cmake --build --preset default` plus the delegated focused
  proof.
- Compare baseline rows by stable test name whenever referencing broad logs.

## Steps

### Step 1: Establish Runtime/Semantic Proof Harness

Goal: Produce a focused proof path for the generated RV64 object.

Actions:

- Inspect the existing backend object execution or emulation harnesses for RV64
  object semantics.
- If a repo-native harness exists, adapt it to the focus case without changing
  expectations or baseline policy.
- If no harness exists, compare generated object behavior against a known-good
  clang object or record the exact missing harness blocker.
- Save commands, object paths, disassembly, relocations, and observed result
  under `build/agent_state/676_step1_runtime_contract/`.

Completion Check:

- The runbook has a concrete command or documented blocker for observing
  whether the generated object returns the expected global short value.

### Step 2: Decide Valid Object Versus Fresh Implementation Owner

Goal: Classify the row from semantic proof, not from stale expected-fail state.

Actions:

- Run the focused runtime/semantic proof when a harness is available.
- For a valid object, document the stale expected-failure/test-contract owner
  and leave expectation or baseline edits for an explicitly delegated route.
- For an invalid object, capture the first failing semantic boundary with
  object bytes, relocations, and runtime behavior.
- Do not infer validity from the expected-failure wrapper alone.

Completion Check:

- The row is assigned to either stale test-contract/baseline routing or a
  precise implementation owner backed by runtime evidence.

### Step 3: Prove Focused Outcome And Hand Back

Goal: Leave the supervisor with an unambiguous next route.

Actions:

- Run `cmake --build --preset default` when code changed, plus the delegated
  focused proof command.
- Update `todo.md` with the latest proof, owner decision, and any blocker.
- If the row is contract-only, recommend routing to the supervisor without
  editing expectations in this packet.
- If the row needs implementation, recommend the smallest focused follow-up or
  continue only within this idea's scoped owner.

Completion Check:

- The focused row has a documented proof-backed route, and
  `test_baseline.new.log` remains diagnostic evidence rather than accepted
  baseline state.
