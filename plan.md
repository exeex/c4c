# Shared Prepared Dynamic Stack Source Authority Runbook

Status: Active
Source Idea: ideas/open/44_shared_prepared_dynamic_stack_source_authority.md

## Purpose

Define target-neutral prepared authority for dynamic-address stack-source
edge-publication loads before any target lowers them as `StackSlot -> Register`
loads.

## Goal

Represent or precisely fail-close one dynamic stack-source publication family
through shared prepared facts, without letting RISC-V infer stack loads from
target-local scans, pointer-base decorations, fixture shape, or missing
concrete offsets.

## Core Rule

Dynamic stack-source lowering must be authorized by shared prepared records
that identify the base anchor, address expression, source memory access, load
width, validity status, and scratch requirements. Do not assume `sp` plus an
offset when shared prepared facts do not provide that address contract.

## Read First

- `ideas/open/44_shared_prepared_dynamic_stack_source_authority.md`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`

## Current Scope

- Inventory dynamic stack-source shapes that reach prepared edge-publication
  planning.
- Define the minimal shared prepared authority for one selected dynamic
  stack-source family, or record the precise upstream producer gap.
- Preserve existing concrete-offset stack-source behavior, including
  large-offset materialization.
- Update RISC-V only after shared facts make a dynamic form safe to consume.

## Non-Goals

- Do not add typed scalar extension or register-bank policy for concrete
  offsets.
- Do not add aggregate stack-source copy policy.
- Do not broaden pointer-base publication behavior unrelated to dynamic
  stack-source authority.
- Do not perform broad frame-layout or dynamic-stack rewrites.
- Do not add target-local predecessor or successor block scans.

## Working Model

- Shared prepare owns semantic authority for whether a dynamic stack-source
  publication is valid, what address it reads, and what scratch resources are
  required.
- RISC-V owns target-local instruction spelling only after shared prepared
  authority describes a safe load.
- Missing or incomplete dynamic-address facts should stay unsupported and
  fail closed.

## Execution Rules

- Prefer explicit shared prepared fields or query helpers over target-local
  rediscovery.
- Keep the first supported or fail-closed dynamic family narrow and named in
  `todo.md` before implementation proceeds.
- Treat expectation weakening, fixture-shaped matching, block scans, or
  pointer-base reclassification as route failure.
- For code-changing steps, prove with a fresh build plus focused prepared
  edge-publication tests; escalate to broader backend validation when shared
  prepared structures or lookup contracts change.

## Steps

### Step 1: Audit Dynamic Stack-Source Authority

Goal: identify which dynamic stack-source shapes reach prepared
edge-publication planning and what authority is currently available for each.

Primary targets:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

Actions:

- Trace how prepared edge-publication records represent stack sources with no
  concrete `offset_bytes`.
- Classify each observed dynamic shape by base anchor, address expression,
  source memory access, load width, validity status, and scratch needs.
- Choose one dynamic stack-source family for shared authority work, or record
  the exact upstream producer gap that prevents safe representation.
- Confirm existing concrete-offset and large-offset stack-source paths do not
  depend on dynamic-source assumptions.

Completion check:

- `todo.md` records the selected dynamic family or precise producer gap, with
  the next step narrowed to shared prepared authority rather than RISC-V
  inference.

### Step 2: Add Shared Prepared Dynamic Stack-Source Authority

Goal: represent the selected dynamic stack-source family in shared prepared
facts, or make its fail-closed diagnostic explicit and precise.

Primary targets:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- focused prepared lookup or printer tests as needed

Actions:

- Add the minimal target-neutral carrier fields or query helpers for base
  anchor, address expression, memory access, load width, validity, and scratch
  requirements.
- Populate those fields only where the prepared pipeline has real authority.
- Preserve unavailable and incomplete states distinctly so consumers can fail
  closed.
- Update printer or lookup coverage if the shared fact shape becomes
  externally observable.

Completion check:

- Shared prepared facts expose the selected dynamic authority without target
  opcode or register spelling decisions, or diagnostics identify the missing
  upstream producer facts exactly.

### Step 3: Consume Shared Dynamic Authority In RISC-V

Goal: update RISC-V to lower the selected dynamic stack-source family only
when shared prepared authority is complete.

Primary targets:

- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

Actions:

- Extend the RISC-V edge-publication move intent only with facts copied from
  shared prepared authority.
- Materialize the target-local address sequence from the shared base/address
  contract and scratch requirements.
- Keep unsupported dynamic neighbors fail-closed when any required shared fact
  is missing or invalid.
- Verify that clearing shared lookup/publication authority prevents RISC-V
  from rediscovering the source through block scans or pointer-base facts.

Completion check:

- The selected RISC-V dynamic `StackSlot -> Register` path emits only from
  shared prepared authority, and negative cases prove it is not inferred from
  missing offsets, pointer-base decorations, fixture names, value ids, or block
  scans.

### Step 4: Validate Existing Supported And Fail-Closed Behavior

Goal: prove the dynamic authority work did not weaken concrete stack-source
support or fail-closed guardrails.

Primary targets:

- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- relevant CMake/backend test entries

Actions:

- Preserve existing concrete-offset and large-offset stack-source coverage.
- Add or keep focused negative tests for unsupported dynamic forms.
- Run the focused build/test proof chosen by the supervisor.
- Run broader backend validation if shared prepared layout or lookup behavior
  changed outside the focused RISC-V consumer.

Completion check:

- Focused tests and required backend validation pass without expectation
  weakening, and `todo.md` records the exact proof command and result.
