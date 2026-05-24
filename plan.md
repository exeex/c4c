# Prealloc Runtime Carrier Naming Alignment Plan

Status: Active
Source Idea: ideas/open/prealloc-runtime-carrier-naming-alignment.md

## Purpose

Make runtime-helper and special-carrier ownership visible through names,
comments, grouping, and prepared-printer terminology without changing helper
selection, marshaling, carrier requirements, intrinsic support, atomics,
inline-asm behavior, or prepared dump meaning.

## Goal

Separate runtime helper-call resource terminology from special carrier fact
terminology while preserving the existing aggregate contracts.

## Core Rule

This is a behavior-preserving naming and contract-clarity plan. Do not merge
runtime helper resources with special carrier facts, and do not make semantic
support changes under a naming label.

## Read First

- `ideas/open/prealloc-runtime-carrier-naming-alignment.md`
- `src/backend/prealloc/runtime_helpers.hpp`
- `src/backend/prealloc/special_carriers.hpp`
- `src/backend/prealloc/prepared_printer/runtime_helpers.cpp`
- `src/backend/prealloc/prepared_printer/special_carriers.cpp`

## Current Targets

- `src/backend/prealloc/runtime_helpers.hpp`
- `src/backend/prealloc/i128_runtime_helpers.cpp`
- `src/backend/prealloc/i128_runtime_helpers.hpp`
- `src/backend/prealloc/f128_runtime_helpers.cpp`
- `src/backend/prealloc/f128_runtime_helpers.hpp`
- `src/backend/prealloc/special_carriers.cpp`
- `src/backend/prealloc/special_carriers.hpp`
- `src/backend/prealloc/atomics.cpp`
- `src/backend/prealloc/atomics.hpp`
- `src/backend/prealloc/intrinsics.cpp`
- `src/backend/prealloc/intrinsics.hpp`
- `src/backend/prealloc/inline_asm.cpp`
- `src/backend/prealloc/inline_asm.hpp`
- `src/backend/prealloc/prepared_printer/runtime_helpers.cpp`
- `src/backend/prealloc/prepared_printer/special_carriers.cpp`

## Non-Goals

- Do not merge `runtime_helpers.hpp` and `special_carriers.hpp`.
- Do not alter helper-call selection, marshal behavior, carrier requirements,
  intrinsic support decisions, atomics, or inline-asm behavior.
- Do not create generic helper dispatch that erases i128, f128, atomics,
  intrinsics, or inline-asm differences.
- Do not rewrite supported/unsupported expectations or weaken prepared dumps.
- Do not move target ABI policy out of its current owner as part of naming work.

## Working Model

- Runtime helpers own helper-call resources and marshal plans for concrete
  helper families such as i128, f128, atomics, intrinsics, and inline asm.
- Special carriers own value-carrier facts that must be visible to later
  prealloc phases.
- Prepared printers mirror the data contracts; they must not invent a second
  ownership taxonomy.

## Execution Rules

- Start with an inventory packet before renaming anything.
- Prefer comment and local helper-name repairs before public contract churn.
- Keep each code-changing packet small enough to prove with the backend subset.
- For any renamed public data concept, update matching prepared-printer labels
  or explicitly record why the printed name is intentionally unchanged.
- If a candidate rename would force semantic changes, record the blocker in
  `todo.md` and leave the behavior alone.
- Every code-changing step must run:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'
```

- Every packet must also run `git diff --check`.

## Steps

### Step 1 - Inventory Runtime And Carrier Terminology

Goal: map the current names, comments, printer labels, and data-family
boundaries before changing contracts.

Primary targets:

- `src/backend/prealloc/runtime_helpers.hpp`
- `src/backend/prealloc/special_carriers.hpp`
- `src/backend/prealloc/prepared_printer/runtime_helpers.cpp`
- `src/backend/prealloc/prepared_printer/special_carriers.cpp`

Actions:

- Inspect runtime helper and special carrier structs, enums, helper functions,
  comments, and printer labels.
- Identify terminology that confuses helper-call resources with value-carrier
  facts.
- Classify likely next packets as comment-only, file-local rename, public data
  rename, or printer mirror alignment.
- Record any names that must stay unchanged because they are established dump
  vocabulary or public aggregate contract.

Completion check:

- `todo.md` records the terminology map, safest first code packet, watchouts,
  and `git diff --check` proof for the no-code inventory.

### Step 2 - Clarify Runtime Helper Resource Names

Goal: make runtime helper-call resource and marshal-plan names read as helper
contracts without changing helper behavior.

Primary targets:

- `src/backend/prealloc/runtime_helpers.hpp`
- `src/backend/prealloc/i128_runtime_helpers.*`
- `src/backend/prealloc/f128_runtime_helpers.*`
- `src/backend/prealloc/atomics.*`
- `src/backend/prealloc/intrinsics.*`
- `src/backend/prealloc/inline_asm.*`
- `src/backend/prealloc/prepared_printer/runtime_helpers.cpp`

Actions:

- Apply one small rename or comment/grouping repair identified by Step 1.
- Keep i128, f128, atomics, intrinsics, and inline-asm ownership visible.
- Update runtime-helper printer terminology only when printed field meaning
  follows the renamed contract.

Completion check:

- The runtime-helper terminology is clearer, no helper selection or marshal
  behavior changes, backend subset passes, and `git diff --check` passes.

### Step 3 - Clarify Special Carrier Fact Names

Goal: make special carrier fact names communicate carrier requirements without
absorbing runtime helper-call resource concepts.

Primary targets:

- `src/backend/prealloc/special_carriers.hpp`
- `src/backend/prealloc/special_carriers.cpp`
- `src/backend/prealloc/prepared_printer/special_carriers.cpp`

Actions:

- Apply one small rename or comment/grouping repair identified by Step 1.
- Keep carrier facts separate from helper-call resources.
- Update special-carrier printer terminology only when it mirrors the renamed
  data contract.

Completion check:

- Carrier terminology is clearer, carrier requirements and support decisions
  are unchanged, backend subset passes, and `git diff --check` passes.

### Step 4 - Align Printer Mirrors

Goal: make prepared-printer terminology mirror the final runtime-helper and
special-carrier data contracts.

Primary targets:

- `src/backend/prealloc/prepared_printer/runtime_helpers.cpp`
- `src/backend/prealloc/prepared_printer/special_carriers.cpp`

Actions:

- Audit printer labels after Steps 2 and 3.
- Align labels or comments that still describe the old ownership model.
- Do not change printed dump meaning or omit existing fields.

Completion check:

- Printer files mirror the data-contract terminology without creating a second
  taxonomy, backend subset passes when code changes, and `git diff --check`
  passes.

### Step 5 - Closure Audit

Goal: decide whether the source idea is complete or whether a narrower follow-up
idea is needed.

Primary targets:

- `todo.md`
- `ideas/open/prealloc-runtime-carrier-naming-alignment.md` only if durable
  lifecycle notes are required by the plan owner during close or split.

Actions:

- Review all terminology changes against the source idea acceptance criteria.
- Confirm no behavior, support expectations, or prepared dump meaning changed.
- If a remaining rename is too broad or semantic, propose a separate open idea
  instead of expanding this plan.

Completion check:

- `todo.md` records closure readiness or the exact follow-up needed, with
  `git diff --check` proof for a no-code audit.
