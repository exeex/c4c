# Prepared Memory Volatility And Address-Space Carrier Runbook

Status: Active
Source Idea: ideas/open/206_prepared_memory_volatility_address_space_carrier.md

## Purpose

Add a shared prepared-memory fact carrier for volatile and non-default
address-space memory semantics so target backends do not infer those facts from
printed BIR, rendered names, or target-local patterns.

Goal: prepared memory consumers can query typed volatility and address-space
facts for memory accesses when BIR can express them.

Core Rule: preserve existing prepared address identity, base, offset, size, and
alignment behavior while adding the missing semantic facts at the shared
preparation boundary.

## Read First

- `ideas/open/206_prepared_memory_volatility_address_space_carrier.md`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `tests/backend/backend_prepare_stack_layout_test.cpp`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`

## Scope

- BIR memory facts: `bir::MemoryAddress::address_space` and
  `bir::MemoryAddress::is_volatile`.
- Shared prepared memory facts: `PreparedMemoryAccess`, `PreparedAddress`, and
  lookup helpers under `src/backend/prealloc/`.
- Prepared-memory construction in stack layout preparation.
- Focused tests or documented fixture proof for volatile and non-default
  address-space preservation.
- Backend-facing docs that define the shared prepared memory carrier contract.

## Non-Goals

- Do not implement AArch64, x86, RISC-V, assembler, object writer, linker, or
  relocation memory lowering.
- Do not invent target-local defaults for missing BIR volatility or address
  spaces.
- Do not parse printed BIR, debug dumps, rendered operand text, source
  spellings, or testcase names to recover memory semantics.
- Do not redesign `PreparedBirModule` beyond the prepared memory carrier needed
  here.
- Do not claim atomics, memory ordering, inline assembly memory clobbers, or
  alias analysis complete under this idea.
- Do not weaken, skip, reclassify, or expectation-rewrite backend tests to make
  target memory lowering appear supported.

## Working Model

- BIR already has typed memory facts on `bir::MemoryAddress`.
- Prepared addressing currently publishes memory access identity and address
  shape but does not expose volatility or address-space fields to backend
  consumers.
- The implementation should carry these facts through the existing prepared
  memory access path, using typed fields or helpers that backend targets can
  query directly.
- If the current frontend or LIR-to-BIR route cannot produce one of the facts,
  document that gap in the proof while still preserving the fact when a BIR
  fixture provides it.

## Execution Rules

- Keep each code-changing step buildable.
- Prefer small tests around `PreparedMemoryAccess` construction and lookup
  before touching target code.
- Preserve designated-initializer compatibility by updating all
  `PreparedMemoryAccess` construction sites deliberately.
- Treat any target-only handling of volatility or address spaces as route drift.
- Update docs only after the typed prepared fields or helpers exist.

## Ordered Steps

### Step 1: Audit Existing BIR And Prepared Memory Surfaces

Goal: identify the exact source facts, prepared structs, builders, helpers, and
tests affected by the carrier extension.

Primary targets:
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `tests/backend/backend_prepare_stack_layout_test.cpp`

Actions:
- Inspect `bir::MemoryAddress` and memory instruction address fields for
  existing volatility and address-space facts.
- Inspect `PreparedAddress`, `PreparedMemoryAccess`, and lookup helpers for
  current prepared-memory publication behavior.
- List every `PreparedMemoryAccess` construction site and every test helper
  that will need field updates.
- Determine whether volatile and non-default address-space facts can be
  produced by current test fixtures or must be injected through BIR fixtures.

Completion check:
- `todo.md` records the audited source facts, construction sites, test targets,
  and any fixture-production limitation before implementation begins.

### Step 2: Extend The Shared Prepared Carrier

Goal: add typed prepared fields or helper-accessible facts for volatility and
address space without changing existing base, offset, size, alignment, or
identity semantics.

Primary targets:
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`

Actions:
- Add explicit prepared volatility and address-space facts in the shared
  memory-access carrier.
- Thread the facts from `bir::MemoryAddress` through all prepared memory access
  builders.
- Preserve existing behavior for frame-slot, global-symbol, pointer-value, and
  string-constant bases.
- Update lookup/helper APIs only where needed to keep backend consumers on typed
  prepared data.

Completion check:
- The repo builds, all `PreparedMemoryAccess` construction sites compile, and
  existing prepared-memory tests still pass.

### Step 3: Prove Fact Preservation

Goal: prove prepared memory construction preserves volatile and address-space
facts where BIR expresses them.

Primary targets:
- `tests/backend/backend_prepare_stack_layout_test.cpp`
- nearby prepared-memory tests if a narrower fixture already exists

Actions:
- Add focused coverage for at least one volatile memory access.
- Add focused coverage for at least one non-default address-space access, or
  document why current frontend/BIR production cannot yet create one and prove
  preservation with a direct BIR fixture if possible.
- Cover representative prepared bases from the source idea when feasible:
  frame slot, global symbol, pointer value, and string constant.
- Ensure tests assert typed prepared fields, not text output.

Completion check:
- The targeted backend prepared-memory test subset passes and the proof command
  is recorded in `todo.md`.

### Step 4: Update Prepared Carrier Documentation

Goal: make the backend-facing contract name the exact prepared fields or helpers
targets should consume before implementing memory lowering.

Primary targets:
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/CLASSIFICATION_INDEX.md`
- nearby prepared/backend documentation if it already describes
  `PreparedMemoryAccess`

Actions:
- Replace the AArch64 ledger gap status with the implemented shared-carrier
  contract, while leaving target-local memory lowering out of scope.
- Name the exact prepared fields or helpers carrying volatility and address
  space.
- Keep deferred target MIR, assembler, object, linker, atomics, inline asm, and
  alias-analysis work explicitly deferred.

Completion check:
- Documentation matches the implemented field/helper names and no longer says
  the shared prepared carrier is missing.

### Step 5: Acceptance Validation

Goal: prove the completed carrier is coherent and has not regressed prepared
memory behavior.

Actions:
- Run the focused prepared-memory test subset used during implementation.
- Run a build or broader backend subset chosen by the supervisor for the final
  acceptance confidence.
- Run `git diff --check`.
- If the diff or proof suggests target-only handling, testcase overfit, or
  expectation downgrades, stop for reviewer scrutiny before closure.

Completion check:
- `todo.md` records fresh validation results and any residual documented BIR
  production gaps. The source idea is ready for close review only if the
  acceptance criteria in `ideas/open/206_prepared_memory_volatility_address_space_carrier.md`
  are satisfied.
