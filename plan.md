# Prepared Select Chain Dump Contract Coverage Runbook

Status: Active
Source Idea: ideas/open/108_prepared_select_chain_dump_contract_coverage.md

## Purpose

Add prepared-printer and contract-test visibility for target-significant
select-chain facts that prealloc already publishes for backend consumers.

## Goal

Expose scalar select-chain materialization, direct-global select-chain
dependency, and directly supporting source-producer provenance facts in
prepared dumps, with focused tests that fail if those consumer-facing facts
disappear.

## Core Rule

Do not rewrite prepared lookup authority or target lowering policy. This source
idea is about dump and contract-test visibility for facts already available to
backend prepared lookup consumers.

## Read First

- `ideas/open/108_prepared_select_chain_dump_contract_coverage.md`
- `src/backend/prealloc/prepared_printer.*`
- `src/backend/prealloc/prepared_printer/*`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- Existing prepared lookup users for scalar select-chain materialization and
  direct-global select-chain dependency

## Current Targets

- `find_prepared_scalar_select_chain_materialization` dump visibility:
  availability, root value name, `root_is_select`, and root instruction index.
- `find_prepared_direct_global_select_chain_dependency` dump visibility for
  call, indirect callee, call-argument, and store-source publication consumers.
- Supporting source-producer kind, source block label, and source instruction
  index when they explain the same scalar select-chain or direct-global
  dependency contract.
- Prepared-printer tests that pin the new dump contract.

## Non-Goals

- Do not rewrite prepared lookup APIs.
- Do not move select-chain materialization or target emission policy into BIR.
- Do not change AArch64, x86, or RISC-V lowering behavior except as needed to
  keep existing tests compiling.
- Do not create compare-join continuation or select-materialization
  join-transfer work.
- Do not broaden prepared-printer output for unrelated publication facts.

## Working Model

The control publication lookup audit found no duplicated BIR control authority
in scalar select-chain and direct-global dependency paths. These are legitimate
prepared target-facing facts consumed by backends. The gap is that prepared
dumps do not expose the lookup-visible fields, leaving future backend rebuild
work dependent on implicit lookup behavior instead of a contract-visible dump.

## Execution Rules

- Keep inventory and proof notes in `todo.md`.
- Add dump output only for facts tied to scalar select-chain materialization or
  direct-global select-chain dependency visibility.
- Print source-producer kind/block/index only as supporting provenance for
  those facts.
- Preserve existing dump expectations unless intentionally extending them.
- For code-changing steps, run `cmake --build --preset default` before handing
  the packet back.
- Focus proof on `backend_prepared_printer` and escalate to broader backend
  validation if shared printer or prepared lookup interfaces change.

## Step 1: Inventory Lookup Facts And Existing Dump Coverage

Goal: identify the exact select-chain and direct-global facts available to
prepared lookup consumers and which are currently missing from prepared dumps.

Primary targets:

- Prepared lookup definitions for scalar select-chain materialization
- Prepared lookup definitions for direct-global select-chain dependency
- `src/backend/prealloc/prepared_printer.*`
- `src/backend/prealloc/prepared_printer/*`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:

- Trace scalar select-chain materialization fields: availability, root value,
  root select status, and root instruction index.
- Trace direct-global select-chain dependency fields used by call, indirect
  callee, call-argument, and store-source publication consumers.
- Trace source-producer kind, block label, and instruction index fields that
  directly support those contracts.
- Compare the available facts to existing prepared-printer output and tests.
- Record missing dump fields and candidate test fixtures in `todo.md`.

Completion check:

- `todo.md` lists the exact fields to expose, current dump gaps, and candidate
  focused tests.
- Analysis proof confirms no implementation diff under `src/backend/prealloc`
  or `tests/backend`.

## Step 2: Design The Dump Contract

Goal: decide where and how the prepared-printer output should expose the
select-chain and direct-global dependency facts.

Actions:

- Choose stable dump labels for scalar select-chain materialization fields.
- Choose stable dump labels for direct-global select-chain dependency fields.
- Decide where source-producer kind/block/index belongs as supporting
  provenance.
- Keep formatting compatible with existing prepared-printer style.
- Reject broad unrelated dump expansion.

Completion check:

- `todo.md` records the output contract, implementation targets, and focused
  test assertions.
- Analysis proof confirms no implementation diff unless this packet
  intentionally begins implementation.

## Step 3: Implement Prepared Dump Visibility

Goal: extend prepared-printer output for the selected consumer-facing facts.

Actions:

- Print scalar select-chain materialization availability and root fields.
- Print direct-global select-chain dependency fields for relevant publication
  consumers.
- Print source-producer kind/block/index only where it directly supports the
  same contract.
- Preserve existing dump sections and stable formatting.
- Avoid changes to prepared lookup authority or target lowering behavior.

Completion check:

- The default build passes.
- The diff is limited to prepared-printer visibility and any narrow fixture
  support needed for tests.
- Existing prepared-printer expectations are extended rather than weakened.

## Step 4: Add Focused Prepared-Printer Tests

Goal: prove dump visibility for scalar select-chain materialization,
direct-global select-chain dependency, and supporting source-producer
provenance.

Actions:

- Add or strengthen a prepared-printer test for scalar select-chain
  materialization fields.
- Add or strengthen a prepared-printer test for direct-global select-chain
  dependency fields.
- Assert source-producer kind/block/index only where it directly supports the
  selected contract.
- Use an existing fixture when possible; add a narrow supporting fixture only
  if needed.

Completion check:

- The default build passes.
- `backend_prepared_printer` or the relevant prepared-printer subset passes.
- Tests fail if the selected facts disappear from the dump.

## Step 5: Final Validation And Close Readiness

Goal: prove the completed route and prepare the source idea for closure review.

Actions:

- Run the default build and the prepared-printer test subset.
- Include broader `^backend_` validation if shared printer or prepared lookup
  interfaces changed.
- Update `todo.md` with final proof and close-readiness notes.

Completion check:

- Final delegated proof passes.
- `todo.md` shows coverage for scalar select-chain materialization,
  direct-global select-chain dependency, and supporting source-producer
  provenance.
- The source idea acceptance criteria are satisfied without lookup API rewrites,
  target lowering changes, or unrelated dump expansion.
