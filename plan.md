# BIR Address Projection Model Consolidation Runbook

Status: Active
Source Idea: ideas/open/08_bir-address-projection-model-consolidation.md

## Purpose

Make local and global GEP families use the same shared BIR address projection
vocabulary where their semantics match, without changing lowering behavior.

Goal: consolidate repeated memory address/projection reasoning around existing
memory helper surfaces while keeping caller-specific policy at the call sites.

Core Rule: preserve BIR output, diagnostics, and testcase expectations.

## Read First

- `ideas/open/08_bir-address-projection-model-consolidation.md`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
- related provenance and scalar subobject memory lowering code

## Current Scope

- Inventory projection facts shared by local GEP, global/relative GEP, and
  provenance scalar subobject paths.
- Strengthen existing helper result types only when a caller needs an already
  computed projection fact.
- Route equivalent aggregate traversal through shared pure helpers.
- Normalize naming around byte offsets, child type/index, element stride,
  repeated extent, and scalar leaf facts.
- Prove the refactor with build and relevant BIR/LIR-to-BIR tests.

## Non-Goals

- Do not redesign BIR memory semantics.
- Do not introduce `MemoryLoweringState`.
- Do not convert stateful lowering to free functions taking
  `BirFunctionLowerer& self`.
- Do not combine this with load/store extraction.
- Do not add per-family headers.
- Do not add new `.hpp` files unless this plan is explicitly revised with a
  concrete justification.
- Do not rewrite testcase expectations.
- Do not perform testcase-shaped special casing.

## Working Model

- `BirFunctionLowerer` remains the owner of stateful lowering operations.
- `memory_helpers.hpp` remains a pure helper declaration surface.
- Existing helpers such as `resolve_aggregate_byte_offset_projection`,
  `resolve_scalar_layout_facts_at_byte_offset`, and
  `can_reinterpret_byte_storage_as_type` are the preferred vocabulary anchors.
- Shared helpers may compute semantic facts; local/global/provenance call sites
  still own their policy decisions.

## Execution Rules

- Keep each code-changing step behavior-preserving.
- Prefer small helper result extensions over new abstractions.
- Keep implementation edits close to the memory lowering files named above.
- When a step touches shared helpers, run at least a build plus the narrow tests
  that exercise the affected GEP/provenance/memory paths.
- Escalate to broader BIR/LIR-to-BIR validation after helper facts are reused
  across more than one caller family.
- Treat expectation rewrites or named-case shortcuts as route drift, not proof.

## Ordered Steps

### Step 1. Inventory Projection Paths

Goal: map equivalent projection reasoning before changing helper contracts.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- provenance scalar subobject paths under BIR memory lowering

Actions:

- Inspect local GEP projection logic, global/relative GEP projection logic, and
  scalar subobject provenance checks.
- Identify repeated layout walks, byte-offset calculations, repeated extent
  handling, and scalar leaf fact checks.
- Separate shared semantic facts from caller-specific local/global/provenance
  policy.
- Record the smallest viable helper/result-type targets in `todo.md` before
  implementation starts.

Completion check:

- `todo.md` names the repeated facts to consolidate and the files expected to
  change first.
- No implementation files are edited in this step unless the executor packet
  explicitly includes the first behavior-preserving extraction.

### Step 2. Strengthen Helper Result Facts

Goal: expose missing projection facts through existing pure helper surfaces.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- matching helper implementation files

Actions:

- Add only the result fields needed by the Step 1 inventory.
- Prefer extending existing result structs over introducing new headers or
  parallel helper names.
- Keep helper inputs explicit and argument-driven.
- Keep all lowerer-state mutation outside helper functions.

Completion check:

- `c4c_codegen` builds.
- Existing tests covering the touched helper callers still pass.
- Call sites can use the new facts without duplicating the same layout walk.

### Step 3. Reuse Shared Projection Helpers

Goal: make equivalent local/global aggregate traversal rely on the same helper
facts while preserving call-site policy.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
- provenance memory lowering code identified in Step 1

Actions:

- Replace duplicate aggregate traversal or offset-resolution code with calls to
  the shared helper vocabulary.
- Keep decisions that differ between local, global, relative, and provenance
  paths in their current implementation files.
- Avoid broad movement of lowering state or unrelated load/store behavior.

Completion check:

- Equivalent aggregate projection paths consume shared helper facts.
- Local/global/provenance-specific decisions remain readable at their call
  sites.
- `c4c_codegen` builds and relevant GEP/provenance/memory tests pass.

### Step 4. Normalize Projection Naming

Goal: remove near-duplicate names for the same projection facts where the
shared model is now clear.

Primary targets:

- helper result fields and helper-local variable names
- local/global/provenance call-site variable names touched by earlier steps

Actions:

- Use consistent names for byte offset, child type, child index, element stride,
  repeated extent, and scalar leaf facts.
- Rename only within the active projection consolidation surface.
- Avoid cosmetic churn outside the files already touched for this plan.

Completion check:

- Naming in helper declarations, helper implementations, and updated call sites
  uses the same terms for the same facts.
- Build and narrow tests remain green.

### Step 5. Final Behavior-Preservation Proof

Goal: prove the consolidation did not change compiler behavior.

Actions:

- Build `c4c_codegen`.
- Run relevant BIR/LIR-to-BIR GEP, aggregate, provenance, and memory tests.
- Escalate to a broader repo-native check if multiple memory caller families
  changed or the supervisor asks for milestone confidence.
- Do not rewrite expectations.

Completion check:

- The build passes.
- Relevant tests pass with no expectation rewrites.
- `todo.md` records the final proof commands and results.
