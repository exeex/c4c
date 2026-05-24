# Prealloc Store-Source Publication Planning Runbook

Status: Active
Source Idea: ideas/open/prealloc-store-source-publication-planning.md

## Purpose

Move target-neutral store-source planning facts into prealloc while keeping final
store lowering and address legality in target codegen.

## Goal

Expose a reusable prealloc store-source plan for source provenance,
recovered-source lookup, stack-object identity, pending publication, and
pointer-store writeback intent, then consume it from AArch64 without changing
emitted behavior.

## Core Rule

Prealloc may describe logical store-source intent. It must not own AArch64
memory operands, store mnemonics, stack-pointer sequences, global address
materialization, scratch register choice, or final machine instruction records.

## Read First

- `ideas/open/prealloc-store-source-publication-planning.md`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_store_sources.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `tests/backend/mir/CMakeLists.txt`

## Current Targets

- Prealloc-owned store-source planning record and helper APIs.
- AArch64 store-source lowering call sites that still derive target-neutral
  source, recovered-source, stack-object, pending-publication, or pointer
  writeback facts locally.
- Focused backend tests proving behavior preservation and cross-target record
  reuse.

## Non-Goals

- Do not move target-specific address-mode legality or memory operand spelling
  into prealloc.
- Do not move AArch64 store opcode selection, ABI lane policy, scratch register
  choice, or instruction construction into prealloc.
- Do not change scalar publication planning unrelated to store-source
  requirements.
- Do not touch branch fusion, dynamic-stack operations, or call-boundary
  republication.
- Do not weaken existing store expectations or mark supported behavior
  unsupported.

## Working Model

The store-source plan should answer policy-free questions before final target
emission: what logical value is being stored, whether a recovered wide-load
source exists, which prepared stack object or destination identity applies,
whether publication is pending, and whether pointer-store writeback is required.

AArch64 adapters may translate those neutral facts into target-local loads,
stores, address operands, scratch use, publication calls, and final instruction
records.

## Execution Rules

- Use `c4c-clang-tools` before editing C++ store-source or prealloc code.
- Keep each implementation packet behavior-preserving unless the source idea
  explicitly requires a semantic repair.
- Add or update focused tests with the first code slice that introduces a new
  public prealloc record/helper.
- For code-changing steps, run:
  `bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`
- Run `git diff --check` before handing back any code-changing packet.
- Treat testcase-shaped store fixes, expectation downgrades, or AArch64 operand
  bundles renamed as prealloc records as route failures.

## Step 1: Map Store-Source Planning Surfaces

Goal: identify the exact AArch64 and prealloc surfaces that currently own
target-neutral store-source facts.

Primary targets:
- `src/backend/mir/aarch64/codegen/dispatch_store_sources.cpp`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`

Actions:
- Inspect current local/global store source helpers, recovered-source lookup,
  stack-object identity resolution, pending publication checks, and pointer
  writeback paths.
- Classify each fact as target-neutral plan data, AArch64 address/emission
  policy, or a boundary requiring a narrow adapter.
- Record the smallest safe Step 2 extraction candidate in `todo.md`.

Completion check:
- `todo.md` names the mapped helpers, classification results, and the first
  implementation slice.
- No implementation files are changed for this audit-only step.

## Step 2: Add Prealloc Store-Source Plan Record

Goal: add a neutral record/helper that represents store-source planning facts
without target operand or instruction data.

Primary targets:
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/mir/CMakeLists.txt`
- focused backend MIR test file

Actions:
- Define a prealloc-owned store-source planning record for logical source,
  destination identity, recovered wide-load source, stack-object identity,
  pending publication requirement, and pointer-store writeback intent as
  justified by Step 1.
- Add a helper that derives the record from existing Prepared/BIR facts without
  depending on AArch64 codegen headers.
- Add focused record-shape tests that exercise the neutral facts.
- Keep target-specific memory operands, mnemonics, scratch choices, and machine
  instruction records out of the new API.

Completion check:
- The new prealloc API builds and has focused backend test coverage.
- Backend tests pass with the delegated proof command.
- `git diff --check` passes.

## Step 3: Consume Store-Source Plan From AArch64

Goal: route selected AArch64 store-source paths through the neutral plan while
preserving emitted behavior.

Primary target:
- `src/backend/mir/aarch64/codegen/dispatch_store_sources.cpp`

Actions:
- Add narrow AArch64 adapter code that consumes the prealloc store-source plan.
- Migrate local/global store source, recovered-source, pending-publication, or
  pointer-writeback logic in small packets, starting with the lowest-risk
  Step 1 candidate.
- Keep address operand construction, store opcode selection, scratch handling,
  publication emission, and final instruction construction target-local.
- Stop and record a follow-up in `todo.md` if a path requires a broader
  target-neutral hook than this idea owns.

Completion check:
- Touched AArch64 paths consume neutral plan facts through target-local
  adapters.
- Existing AArch64 behavior for touched local/global store, recovered-source,
  and pointer-writeback paths remains equivalent.
- Backend tests pass with the delegated proof command.
- `git diff --check` passes.

## Step 4: Prove Cross-Target Reuse

Goal: demonstrate that another target can inspect the store-source plan without
inheriting AArch64 addressing rules.

Primary targets:
- `tests/backend/mir/CMakeLists.txt`
- x86 or RISC-V focused backend MIR test file

Actions:
- Add an x86- or RISC-V-labeled fixture or adapter test that consumes the
  prealloc store-source plan.
- Cover the important neutral facts introduced by Step 2, including recovered
  source or pointer-writeback intent when represented.
- Do not include AArch64 codegen headers, AArch64 memory operands, or machine
  instruction records in the reuse proof.

Completion check:
- The cross-target test proves record-level reuse without claiming full
  x86/RISC-V lowering support.
- Backend tests pass with the delegated proof command.
- `git diff --check` passes.

## Step 5: Closure Review

Goal: decide whether the source idea is complete or whether remaining store
planning work needs a separate follow-up idea.

Actions:
- Compare completed implementation against the source idea acceptance criteria
  and reviewer reject signals.
- Confirm remaining target-local store emission details are documented as
  non-blockers or create a separate `ideas/open/*.md` follow-up if a distinct
  initiative remains.
- Ask the plan owner to close only if the source idea itself is complete.

Completion check:
- `todo.md` contains a closure recommendation and proof summary.
- Any distinct remaining initiative is recorded under `ideas/open/` before the
  active idea is closed.
