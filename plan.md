# Shared Prepared Aggregate Stack-Source Authority Runbook

Status: Active
Source Idea: ideas/open/45_shared_prepared_aggregate_stack_source_authority.md

## Purpose

Define target-neutral prepared authority for aggregate-width stack-source
edge-publication copies before any target lowers them by byte-size-only scalar
load rules.

## Goal

Represent or precisely fail-close one aggregate stack-source publication
family through shared prepared facts, including copy width, destination/lane
mapping, alignment, partial-copy policy, ABI layout reference, and scratch
ownership.

## Core Rule

Aggregate stack-source lowering must be authorized by shared prepared records
that describe the aggregate copy contract. Do not treat aggregate-sized
`StackSlot -> Register` publications as scalar integer loads based only on
byte width, fixture shape, value id, stack slot id, or offset.

## Read First

- `ideas/open/45_shared_prepared_aggregate_stack_source_authority.md`
- `ideas/closed/42_riscv_prepared_edge_publication_aggregate_stack_source_policy.md`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- relevant target edge-publication consumers under `src/backend/mir/*/codegen/`

## Current Scope

- Audit aggregate stack-source facts currently available from shared prepare.
- Define the minimal shared prepared authority for one aggregate source family.
- Keep instruction selection, concrete register spelling, and target-specific
  copy emission local to each target.
- Update one target consumer only if the selected aggregate family has complete
  shared facts.
- Add focused proof that aggregate stack sources are not lowered as scalar
  loads without explicit aggregate authority.

## Non-Goals

- Do not add typed scalar stack-source support.
- Do not add dynamic-address stack-source support.
- Do not redesign general aggregate ABI layout.
- Do not broaden unrelated source-to-stack destination support.
- Do not treat all aggregate sizes as one scalar integer load.
- Do not move target instruction spelling or concrete register names into
  shared prepared records.

## Working Model

- Shared prepare owns semantic authority for aggregate copy width,
  destination/lane mapping, alignment, partial-copy validity, ABI layout
  reference, and scratch ownership expectations.
- Targets own local register spelling, instruction selection, scratch register
  allocation mechanics, and emission order after shared authority says a copy
  is valid.
- Missing aggregate facts should stay unsupported and fail closed, preserving
  the blocker outcome from idea 42 until real shared authority exists.

## Execution Rules

- Prefer explicit shared prepared fields or query helpers over target-local
  rediscovery.
- Keep the first supported or fail-closed aggregate family narrow and record
  it in `todo.md` before implementation proceeds.
- Treat byte-size-only scalar lowering, fixture-shaped matching, expectation
  weakening, or target-local lane invention as route failure.
- Preserve existing scalar stack-source behavior while aggregate neighbors
  remain separately authorized or fail closed.
- For code-changing steps, prove with a fresh build plus focused prepared
  edge-publication tests; escalate to broader backend validation when shared
  prepared structures or lookup contracts change.

## Steps

### Step 1: Audit Aggregate Stack-Source Authority

Goal: identify which aggregate-width stack-source shapes reach prepared
edge-publication planning and what shared authority is currently available.

Primary targets:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- relevant target edge-publication consumers under `src/backend/mir/*/codegen/`

Actions:

- Trace how prepared edge-publication records represent aggregate-width
  `StackSlot -> Register` sources today.
- Classify observed aggregate shapes by copy width, destination/lane mapping,
  alignment, partial-copy behavior, ABI layout reference, and scratch needs.
- Confirm which scalar stack-source paths are already supported and must remain
  distinct from aggregate authority.
- Choose one aggregate source family for shared authority work, or record the
  exact upstream producer facts still missing.

Completion check:

- `todo.md` records the selected aggregate family or precise producer gap, with
  the next step narrowed to shared prepared authority rather than target-local
  inference.

### Step 2: Add Shared Prepared Aggregate Authority

Goal: represent the selected aggregate stack-source family in shared prepared
facts, or make its fail-closed reason explicit and precise.

Primary targets:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- focused prepared lookup or printer tests as needed

Actions:

- Add the minimal target-neutral carrier fields or query helpers for aggregate
  copy width, lane/destination mapping, alignment, partial-copy validity, ABI
  layout reference, and scratch expectations.
- Populate those facts only where the prepared pipeline has real authority.
- Preserve unavailable and incomplete states distinctly so consumers can fail
  closed.
- Keep target opcode, register spelling, and concrete emission sequence out of
  shared prepared records.
- Update printer or lookup coverage if the shared fact shape becomes
  externally observable.

Completion check:

- Shared prepared facts expose the selected aggregate authority without target
  instruction decisions, or diagnostics identify the missing upstream producer
  facts exactly.

### Step 3: Consume Shared Aggregate Authority In One Target

Goal: update one target consumer to lower the selected aggregate stack-source
family only when shared prepared authority is complete.

Primary targets:

- the chosen target edge-publication consumer under `src/backend/mir/*/codegen/`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` or the
  matching target-focused backend test

Actions:

- Extend the target edge-publication move intent only with facts copied from
  shared prepared authority.
- Emit target-local copy instructions from the shared copy/lane/alignment
  contract and scratch expectations.
- Keep unsupported aggregate neighbors fail-closed when any required shared
  fact is missing or invalid.
- Verify that clearing shared lookup/publication authority prevents the target
  from rediscovering aggregate source facts by byte size, fixture shape,
  offset, value id, or stack slot id.

Completion check:

- The selected target aggregate `StackSlot -> Register` path emits only from
  shared prepared authority, and negative cases prove it is not inferred from
  scalar size rules or local recovery.

### Step 4: Validate Scalar Support And Aggregate Fail-Closed Behavior

Goal: prove aggregate authority work did not weaken existing scalar support or
idea 42 fail-closed guardrails.

Primary targets:

- focused prepared edge-publication backend tests
- relevant CMake/backend test entries

Actions:

- Preserve existing scalar stack-source coverage.
- Add or keep focused negative tests for unsupported aggregate widths, lane
  mappings, partial copies, alignment cases, and scratch requirements.
- Run the focused build/test proof chosen by the supervisor.
- Run broader backend validation if shared prepared layout, lookup behavior,
  or cross-target publication contracts changed.

Completion check:

- Focused tests and required backend validation pass without expectation
  weakening, and `todo.md` records the exact proof command and result.
