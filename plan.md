# Shared Prepared Typed Stack Source Authority Runbook

Status: Active
Source Idea: ideas/open/43_shared_prepared_typed_stack_source_authority.md

## Purpose

Add target-neutral prepared authority for typed scalar `StackSlot -> Register`
edge-publication loads so RISC-V can lower one selected typed form without
guessing from byte size, ids, offsets, fixture names, or raw register spelling.

## Goal

Represent and consume one typed scalar stack-source publication through shared
prepared facts while keeping unsupported neighboring forms explicitly
fail-closed.

## Core Rule

Typed stack-source lowering must be authorized by shared prepared records. Do
not infer signedness, extension behavior, floatingness, or destination register
bank from byte size, value ids, stack slot ids, offsets, fixture names, or raw
register spelling.

## Read First

- `ideas/open/43_shared_prepared_typed_stack_source_authority.md`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`

## Current Scope

- Audit and extend shared prepared edge-publication/value-home facts for
  typed scalar concrete-offset `StackSlot -> Register` sources.
- Update the RISC-V prepared edge-publication consumer for one safe typed
  scalar form only after shared authority exists.
- Preserve existing concrete 4-byte `lw`, 8-byte `ld`, and large-offset
  stack-source behavior.
- Keep dynamic-address and aggregate-width stack-source handling fail-closed.

## Non-Goals

- Do not add dynamic-address stack-source support.
- Do not add aggregate-width stack-source support.
- Do not broaden source-to-`StackSlot` destination handling.
- Do not move target instruction spelling, opcodes, or concrete register names
  into shared prepared records.
- Do not add RISC-V-local producer scans or target-local edge-copy fact tables.

## Working Model

- Shared prepare owns semantic authority: scalar type, extension policy, and
  destination register-bank/view facts when available.
- RISC-V owns target-local emission once shared facts authorize a typed load.
- Missing or incomplete shared facts should produce an unsupported/fail-closed
  status, not a guessed load opcode.

## Execution Rules

- Prefer adding explicit shared prepared fields or query helpers over changing
  RISC-V logic to rediscover semantics.
- Add one narrow positive path first; leave nearby forms covered by negative
  tests unless their shared authority is equally complete.
- Treat expectation weakening or testcase-shaped fixture matching as route
  failure.
- For code-changing steps, prove with a fresh build plus the focused RISC-V
  prepared edge-publication test; escalate to broader backend validation when
  shared prepared structures or lookup contracts change.

## Steps

### Step 1: Audit Typed Stack-Source Authority

Goal: identify exactly which shared prepared facts already exist and which
facts are missing for typed scalar concrete-offset `StackSlot -> Register`
edge-publications.

Primary targets:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

Actions:

- Trace how `PreparedValueHome`, edge-publication lookup records, and RISC-V
  `EdgePublicationMoveIntent` currently carry source stack slot, size, type,
  and destination register information.
- Determine whether scalar type, signedness/extension policy, and destination
  register bank/view are already present, missing, or present only as unsafe
  target-local hints.
- Choose one typed scalar form for the first supported path, or record a
  precise upstream producer gap if no safe form is available.

Completion check:

- `todo.md` records the chosen first typed form or the exact missing producer
  gap, with the next implementation step narrowed to shared prepared authority
  rather than RISC-V inference.

### Step 2: Add Shared Prepared Typed Stack-Source Facts

Goal: make shared prepared records represent the selected typed scalar
stack-source form.

Primary targets:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- focused prepared lookup/printer tests as needed

Actions:

- Add the minimal target-neutral carrier fields or query helpers for scalar
  type, extension/signedness policy, and destination register-bank/view.
- Populate those fields only where the prepared pipeline has real authority.
- Preserve unavailable/incomplete states distinctly so consumers can fail
  closed.
- Update printer or lookup coverage if the shared fact shape becomes
  externally observable.

Completion check:

- Shared prepared facts expose the selected typed scalar authority without
  target opcode or register spelling decisions, and missing facts are
  distinguishable from supported forms.

### Step 3: Consume Shared Typed Authority In RISC-V

Goal: lower one selected typed scalar stack-source publication in RISC-V using
  only shared prepared authority.

Primary targets:

- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

Actions:

- Extend `EdgePublicationMoveIntent` only with facts copied from shared
  prepared authority.
- Select the RISC-V load opcode for the chosen typed form from the shared
  scalar type/extension/bank facts.
- Keep neighboring typed, unsigned-I32-shaped, F32, dynamic, and aggregate
  forms unsupported unless their shared authority is complete in the same
  semantic way.
- Verify that clearing shared lookup/publication authority still prevents
  RISC-V from rediscovering or guessing the move.

Completion check:

- One selected typed scalar `StackSlot -> Register` RISC-V path emits the
  expected target-local instruction sequence, and negative cases prove it is
  not inferred from size, ids, offsets, fixture names, or raw register names.

### Step 4: Validate Existing Supported And Fail-Closed Behavior

Goal: prove the new typed path did not weaken existing concrete stack-source
  support or fail-closed guardrails.

Primary targets:

- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- relevant CMake/backend test entries

Actions:

- Preserve existing 4-byte `lw`, 8-byte `ld`, and large-offset stack-source
  coverage.
- Add or keep focused negative tests for unsupported typed neighboring forms.
- Run the focused build/test proof chosen by the supervisor.
- Run broader backend validation if shared prepared layout or lookup behavior
  changed outside the focused RISC-V consumer.

Completion check:

- Focused tests and required backend validation pass without expectation
  weakening, and `todo.md` records the exact proof command and result.
