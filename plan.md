# AArch64 Returns Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/257_aarch64_returns_markdown_shard_implementation_redistribution.md

## Purpose

Reconcile `src/backend/mir/aarch64/codegen/returns.md` into the compiled
`returns.cpp` and `returns.hpp` ownership surface without changing return ABI
semantics.

Goal: make the returns shard's valid behavior live in ordinary compiled owners
and delete the markdown shard once its durable content is reconciled.

## Core Rule

Move ownership, not behavior. Do not weaken tests, add named-case shortcuts, or
change ABI return semantics to make the redistribution look complete.

## Read First

- `ideas/open/257_aarch64_returns_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/returns.md`
- `src/backend/mir/aarch64/codegen/returns.cpp`
- `src/backend/mir/aarch64/codegen/returns.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`
- `src/backend/mir/ALLOCATION_CONTRACT.md`

## Current Targets

- `src/backend/mir/aarch64/codegen/returns.cpp`
- `src/backend/mir/aarch64/codegen/returns.hpp`
- `src/backend/mir/aarch64/codegen/returns.md`
- broad owners that may still contain returns-family implementation:
  `instruction.cpp`, `dispatch.cpp`, and `machine_printer.cpp`

## Non-Goals

- Do not redistribute any other AArch64 codegen markdown shard.
- Do not redistribute `mod.md`.
- Do not redesign `instruction.hpp` or the instruction type hierarchy solely
  for this split.
- Do not turn `returns.cpp` into a catch-all for non-returns codegen behavior.
- Do not rewrite tests or expectations as a substitute for compiled ownership.

## Working Model

`returns.cpp` should own returns-family selected-node construction, lowering
helpers, and return-specific spelling helpers when those helpers are truly part
of the returns shard. Broad files should route into that owner or retain only
family-neutral infrastructure. Return value homes, before-return moves,
ABI-binding facts, helper-call resources, and return-control facts must remain
structured target MIR facts rather than local register shortcuts.

## Execution Rules

- Keep steps small enough for focused backend proof after each code-changing
  packet.
- Preserve existing ordinary returns, F128 returns, and second floating-point
  return component behavior.
- Use structured records and existing target MIR authority instead of
  testcase-shaped matching.
- If required carrier facts are missing, record the gap in `todo.md` and ask
  the supervisor for a split idea instead of locally inventing semantics.
- Delete `returns.md` only after its valid durable content is reconciled into
  compiled owners or proven stale against existing contracts.

## Ordered Steps

### Step 1: Audit Current Returns Ownership

Goal: identify which valid `returns.md` responsibilities already live in
`returns.cpp`/`returns.hpp` and which remain in broad owners.

Primary target: `returns.md`, `returns.cpp`, `returns.hpp`, `instruction.cpp`,
`dispatch.cpp`, and `machine_printer.cpp`.

Actions:

- Map each `returns.md` entry point, dependency, hidden assumption, and rebuild
  guidance item to current compiled code.
- Classify each item as already owned by `returns.cpp`, still stranded in a
  broad owner, stale under current contracts, or blocked by a missing carrier.
- Inspect existing tests or backend checks that cover ordinary returns, F128
  returns, and second floating-point return components.
- Record the next implementation packet and proof command in `todo.md`.

Completion check:

- `todo.md` names the next code-changing packet, any missing carrier gap, and a
  focused proof command.
- No implementation files are changed in this audit-only step unless the
  supervisor explicitly delegates them in a later executor packet.

### Step 2: Move Return Construction and Lowering Bodies

Goal: ensure returns-family selected-node construction and lowering behavior
is owned by `returns.cpp`/`returns.hpp`.

Primary target: `returns.cpp` and any broad owner currently holding
returns-family bodies.

Actions:

- Move valid return-value materialization and return-control selected-node
  construction into `returns.cpp`/`returns.hpp`.
- Leave only routing or family-neutral instruction core in `dispatch.cpp` and
  `instruction.cpp`.
- Preserve existing diagnostics, origin records, value authority, ABI-binding,
  and allocation-result facts.
- Build after each coherent relocation packet.

Completion check:

- Return construction/lowering entry points are declared in `returns.hpp` and
  implemented in `returns.cpp`.
- Broad owners route to the returns owner or hold only family-neutral code.
- Focused backend proof for affected return lowering remains green.

### Step 3: Move Return-Specific Spelling Helpers

Goal: remove returns-specific spelling bodies from broad printer surfaces when
they can be owned through returns helpers.

Primary target: `returns.cpp`/`returns.hpp` and `machine_printer.cpp`.

Actions:

- Identify instruction spelling logic that is specific to return records or
  return-control nodes.
- Extract that logic into returns-owned helpers without moving unrelated
  printer responsibilities.
- Keep final printer code as routing or generic emission infrastructure.

Completion check:

- `machine_printer.cpp` no longer contains returns-specific spelling bodies
  that belong in the returns shard.
- Printer output for covered return cases is unchanged.
- Focused backend proof remains green.

### Step 4: Reconcile and Delete `returns.md`

Goal: remove the markdown shard after every durable valid item is represented
in compiled owners or explicitly proven stale under current contracts.

Primary target: `src/backend/mir/aarch64/codegen/returns.md`.

Actions:

- Recheck every `returns.md` section against compiled code and contracts.
- Move any remaining valid content into code, comments, or tests only where it
  materially preserves behavior or ownership.
- Delete `returns.md`.
- Record stale or out-of-scope findings in `todo.md` for supervisor review if
  they affect closure judgment.

Completion check:

- `returns.md` is deleted.
- `returns.cpp` and `returns.hpp` own the valid returns shard behavior.
- No durable returns behavior remains only in markdown.

### Step 5: Broader Acceptance Check

Goal: prove the redistribution preserved behavior and did not expand scope.

Primary target: backend validation selected by the supervisor.

Actions:

- Run the focused proof from the final executor packet.
- Escalate to the supervisor-selected broader backend or regression guard
  check because this closes a shard redistribution.
- Confirm the final diff does not include unrelated feature expansion or
  expectation downgrades.

Completion check:

- Focused proof and supervisor-selected broader validation pass.
- The source idea completion criteria are satisfied and ready for lifecycle
  close review.
