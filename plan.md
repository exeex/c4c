# BIR Core Model Cleanup Runbook

Status: Active
Source Idea: ideas/open/518_bir_core_model_cleanup_umbrella.md

## Purpose

Analyze the oversized BIR core model files and produce a staged cleanup plan
without changing compiler behavior.

## Goal

Create a durable BIR core cleanup artifact that inventories current ownership,
maps likely file destinations, and proposes small behavior-preserving follow-up
ideas.

## Core Rule

This runbook is analysis-only. Do not move declarations, move definitions,
rename BIR concepts, change BIR semantics, or edit implementation files as
cleanup progress.

## Read First

- `ideas/open/518_bir_core_model_cleanup_umbrella.md`
- `.codex/skills/c4c-clang-tools/SKILL.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir_printer.cpp`
- `src/backend/bir/bir_validate.cpp`
- `src/backend/bir/lir_to_bir/`

## Current Targets

- Primary model declarations: `src/backend/bir/bir.hpp`
- Primary model implementation: `src/backend/bir/bir.cpp`
- Adjacent ownership checks:
  - `src/backend/bir/bir_printer.cpp`
  - `src/backend/bir/bir_validate.cpp`
  - `src/backend/bir/lir_to_bir/`
- Durable output directory: `docs/bir_core_cleanup/` unless the executor has a
  stronger repo-local reason to use `build/agent_state/518_bir_core_cleanup/`.

## Non-Goals

- Do not implement BIR producer capability.
- Do not move code between files.
- Do not change printer output, validation behavior, prepared handoff, or RV64
  lowering behavior.
- Do not alter gcc_torture expectations, allowlists, unsupported diagnostics,
  pass/fail accounting, or runtime checks.
- Do not fold idea 422 follow-up producer work into this cleanup analysis.

## Working Model

- Treat `bir.hpp` and `bir.cpp` as inventory sources, not edit targets.
- Use AST-backed `c4c-clang-tools` queries before raw long-file reading.
- Separate core IR model ownership from helper algorithms, route-specific
  analysis records, printer-only helpers, validator-only helpers, and
  LIR-to-BIR-only surfaces.
- Prefer existing focused files over new headers or new translation units when
  they already express the right owner.
- Keep broad model type movement late in the proposed follow-up sequence.

## Execution Rules

- Record commands and findings in the durable analysis artifact as execution
  proceeds.
- If AST tooling is unavailable or incomplete, document the attempted command
  and the fallback raw-text method.
- Keep follow-up ideas small, behavior-preserving, and tied to owned files.
- Escalate to supervisor if analysis discovers a separate implementation
  initiative that should become its own source idea.
- For analysis/documentation-only packets, proof may be limited to artifact
  inspection plus a repository sanity check. Code-changing follow-up ideas must
  define build and focused backend proof.

## Ordered Steps

### Step 1: Establish Structure Snapshot

Goal: capture the current size, file shape, and symbol-level structure of the
BIR core model.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `.codex/skills/c4c-clang-tools/SKILL.md`

Actions:

- Confirm `c4c-clang-tool` and `c4c-clang-tool-ccdb` availability.
- Use AST-backed top-level symbol, function signature, caller/callee, and type
  reference queries for `bir.hpp` and `bir.cpp`.
- Record line counts and dependency observations.
- Start the durable artifact under `docs/bir_core_cleanup/` with commands,
  query notes, and initial clusters.

Completion check:

- The artifact records the structure snapshot, the exact commands used, and any
  tooling fallback needed.

### Step 2: Inventory Declaration Families

Goal: classify public model declarations by domain and likely ownership.

Primary target: `src/backend/bir/bir.hpp`

Actions:

- Inventory value, type, module, function, block, and instruction definitions.
- Inventory memory-address and object-storage records.
- Inventory comparison, scalar producer, select-chain, direct-global
  dependency, and compatibility helper surfaces.
- Mark declarations that should remain central because they define the public
  model contract.

Completion check:

- The artifact contains a declaration-family table with current location,
  likely owner, movement risk, and "do not move yet" notes.

### Step 3: Inventory Implementation Families

Goal: classify implementation bodies and helper algorithms without changing
behavior.

Primary targets:

- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir_printer.cpp`
- `src/backend/bir/bir_validate.cpp`
- `src/backend/bir/lir_to_bir/`

Actions:

- Inventory pure helper algorithms currently accumulated in `bir.cpp`.
- Identify printer-only, validator-only, LIR-to-BIR-only, and route-specific
  memory-access analysis implementation families.
- Compare implementation callers with declaration-family ownership.
- Note include-cycle and API compatibility risks.

Completion check:

- The artifact contains an implementation-family map and identifies low-risk,
  medium-risk, and late/no-move regions.

### Step 4: Draft Destination Map

Goal: propose where each family should live if later cleanup work proceeds.

Actions:

- Prefer existing focused files where the owner is already clear.
- Propose new headers or translation units only where existing files would
  preserve the same monolithic coupling.
- Distinguish public model headers from private helper implementation files.
- Keep broad model type moves behind lower-risk helper extraction.

Completion check:

- The artifact contains a destination map that separates existing-file
  redistribution, possible new files, retained central surfaces, and include
  risks.

### Step 5: Produce Staged Follow-Up Ideas

Goal: turn the destination map into small follow-up cleanup ideas.

Actions:

- Draft behavior-preserving follow-up ideas in the recommended execution order.
- Each follow-up idea must name owned files, non-goals, validation expectations,
  and reviewer reject signals.
- Ensure idea 422 producer work remains separate from file-organization
  cleanup.

Completion check:

- The artifact contains a staged follow-up list with concrete owned files,
  validation commands, and rejection criteria.

### Step 6: Finalize Analysis And Lifecycle Handoff

Goal: make the analysis artifact reviewable and ready for supervisor lifecycle
decision.

Actions:

- Verify the artifact satisfies all acceptance criteria from the source idea.
- Check that no implementation files changed as part of this analysis.
- Run an appropriate docs/lifecycle sanity check and report any skipped build
  proof as intentional for analysis-only work.
- Update `todo.md` with the final packet summary and proof.

Completion check:

- The supervisor can decide whether to close, rewrite, or split the idea using
  the completed artifact and `todo.md` summary.
