# Prealloc Stack Layout Contract Boundary Runbook

Status: Active
Source Idea: ideas/open/prealloc-stack-layout-contract-boundary.md

## Purpose

Clarify the stack-layout public boundary and split coordinator internals only
where existing stack/frame responsibilities already separate cleanly.

## Goal

Make stack-layout ownership visible from contract files and helper names while
preserving stack object authority, frame sizing, slot assignment, and prepared
frame dump meaning.

## Core Rule

This is behavior-preserving contract cleanup. Do not change stack layout,
frame size, slot assignment, alignment, inline-asm stack facts, dynamic address
semantics, or final target emission behavior.

## Read First

- `ideas/open/prealloc-stack-layout-contract-boundary.md`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/frame.hpp`
- `src/backend/prealloc/frame_plan.hpp`
- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `src/backend/prealloc/prepared_printer/frame.cpp`

## Current Targets

- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/frame.hpp`
- `src/backend/prealloc/frame_plan.cpp`
- `src/backend/prealloc/frame_plan.hpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `src/backend/prealloc/stack_layout/alloca_coalescing.cpp`
- `src/backend/prealloc/stack_layout/analysis.cpp`
- `src/backend/prealloc/stack_layout/copy_coalescing.cpp`
- `src/backend/prealloc/stack_layout/inline_asm.cpp`
- `src/backend/prealloc/stack_layout/regalloc_helpers.cpp`
- `src/backend/prealloc/stack_layout/slot_assignment.cpp`
- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- `src/backend/prealloc/prepared_printer/frame.cpp`

Potential new stack-layout implementation files such as `objects.*` or
`slots.*` are allowed only after the runbook records a clear coordinator
boundary and the extraction can stay behavior-preserving.

## Non-Goals

- Do not alter stack object authority, frame size, slot assignment, alignment,
  inline-asm stack facts, or dynamic address semantics.
- Do not move final target instruction emission, register spelling, or target
  assembly address formation into prealloc.
- Do not create a forwarding header while `module.hpp` remains the real
  dependency.
- Do not perform broad include churn outside direct stack/frame consumers.
- Do not weaken prepared-printer frame output or hide frame/stack facts.

## Working Model

- `frame.hpp` and `frame_plan.hpp` remain public frame contracts.
- `stack_layout/stack_layout.hpp` is the stack-layout contract surface for
  direct stack-layout consumers.
- `module.hpp` may remain intentionally aggregate when consumers need the
  aggregate module view; it should shed stack-layout declarations only when
  direct consumers can include a real stack-layout contract.
- `stack_layout/coordinator.cpp` should be split only along durable boundaries
  such as object collection, slot assignment orchestration, or address
  materialization/publication.

## Execution Rules

- Keep each code-changing packet small enough to audit against the source idea.
- Prefer file-local helper extraction before creating new headers or new
  implementation files.
- Before moving declarations out of `module.hpp`, inspect all call sites and
  prove they can depend directly on a stack/frame contract.
- If a split would require unclear broad mutable coordinator state, record the
  no-code decision in `todo.md` instead of forcing a new file.
- Preserve prepared-printer terminology whenever a frame or stack data-family
  name changes.
- For every code-changing step, run:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'
```

- Also run `git diff --check` before handing a packet back.

## Step 1 - Inventory Stack And Frame Boundaries

Goal: map the current public declarations, direct consumers, and coordinator
responsibilities before changing files.

Primary targets:
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/frame.hpp`
- `src/backend/prealloc/frame_plan.hpp`
- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `src/backend/prealloc/prepared_printer/frame.cpp`

Actions:
- Inspect stack/frame declarations exposed through `module.hpp` and compare
  them with the frame and stack-layout contract headers.
- Identify direct stack-layout consumers and whether they already include a
  stack/frame contract.
- Classify `stack_layout/coordinator.cpp` logic into object collection, slot
  assignment orchestration, address materialization/publication, and glue.
- Record any prepared-printer naming dependencies that mirror frame/stack
  contract names.

Completion check:
- `todo.md` records the candidate boundary map, no-code decisions, and the
  first safe implementation packet or a reason to defer code.
- `git diff --check` passes.

## Step 2 - Clarify Header Contract Ownership

Goal: make stack-layout-facing declarations live at the smallest real public
contract boundary that existing consumers can use.

Primary targets:
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/frame.hpp`
- `src/backend/prealloc/frame_plan.hpp`
- `src/backend/prealloc/stack_layout/stack_layout.hpp`

Actions:
- Move or document stack-layout-facing declarations exposed through
  `module.hpp` only when direct consumers can include a real stack/frame
  contract.
- Keep `module.hpp` aggregate when consumers still need aggregate module data.
- Avoid forwarding shells and broad include churn.

Completion check:
- Header ownership is clearer without changing public behavior or creating
  fake indirection.
- Backend proof and `git diff --check` pass for any code change.

## Step 3 - Split Coordinator Object And Slot Phases

Goal: reduce unrelated detail in `stack_layout/coordinator.cpp` by extracting
only clear object-collection or slot-assignment phase boundaries.

Primary target:
- `src/backend/prealloc/stack_layout/coordinator.cpp`

Actions:
- Extract file-local helpers first when they reduce broad mutable coordinator
  state.
- Create new stack-layout implementation files only after a helper boundary is
  proven stable and recorded in `todo.md`.
- Preserve object authority, frame sizing, slot order, alignment, and dynamic
  address semantics exactly.

Completion check:
- The coordinator exposes less mixed object/slot detail or `todo.md` explains
  why no split is currently safe.
- Backend proof and `git diff --check` pass for any code change.

## Step 4 - Separate Address Publication From Orchestration

Goal: clarify address-materialization/publication responsibilities without
moving target emission or changing stack address semantics.

Primary targets:
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `src/backend/prealloc/stack_layout/analysis.cpp`
- `src/backend/prealloc/stack_layout/regalloc_helpers.cpp`
- `src/backend/prealloc/stack_layout/slot_assignment.cpp`

Actions:
- Inspect where final stack object locations become published frame facts.
- Extract or rename helpers only when the boundary is stack/frame publication,
  not target assembly address formation.
- Keep final target instruction emission outside prealloc.

Completion check:
- Publication naming or helper boundaries are clearer, or `todo.md` records a
  no-code decision.
- Backend proof and `git diff --check` pass for any code change.

## Step 5 - Align Prepared Frame Printing And Close

Goal: keep debug output aligned with any contract or helper naming changes and
decide whether the source idea is complete.

Primary target:
- `src/backend/prealloc/prepared_printer/frame.cpp`

Actions:
- Update prepared frame printer terminology only if data-family names changed.
- Confirm no frame/stack facts were hidden, dropped, or reclassified.
- Run the backend proof and any supervisor-requested close gate.
- Record any remaining stack/frame cleanup as a separate open idea rather than
  expanding this plan.

Completion check:
- Prepared frame dump meaning is preserved.
- The runbook either closes the source idea or leaves a clear remaining
  lifecycle note for the supervisor.
