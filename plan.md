# LIR To Backend IR Refactor

Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md

## Purpose

Refactor the LIR-to-backend boundary so backend code consumes structured backend
IR semantics through a dedicated lowering layer instead of parsing
LLVM-text-shaped fragments inside target codegen.

## Goal

Land the transition from `lir_adapter` toward explicit LIR-to-backend-IR
lowering without changing backend behavior.

## Core Rule

Keep changes behavior-preserving. Decode legacy LIR text in a narrow lowering
layer; do not expand target emitters' dependence on parser-style
`parse_backend_*` helpers.

## Read First

- [ideas/open/03_lir_to_backend_ir_refactor.md](/workspaces/c4c/ideas/open/03_lir_to_backend_ir_refactor.md)
- [src/backend/lir_adapter.hpp](/workspaces/c4c/src/backend/lir_adapter.hpp)
- [src/backend/lir_adapter.cpp](/workspaces/c4c/src/backend/lir_adapter.cpp)
- [src/backend/ir.hpp](/workspaces/c4c/src/backend/ir.hpp)

## Current Targets

- rename the production boundary around `lir_adapter` into explicit lowering
  entrypoints
- isolate legacy LIR syntax decoding from backend IR construction
- reshape backend IR interfaces toward structured backend-owned semantics
- remove target-codegen dependence on backend text parsing over time

## Non-Goals

- full LIR redesign
- broad semantic changes to backend behavior
- introducing final `bir` naming in this slice
- deleting legacy compatibility code before replacements are proven

## Working Model

1. LIR remains the source IR at this boundary.
2. A dedicated lowering layer decodes any legacy text-heavy LIR fields.
3. Backend IR becomes a backend-owned structured model.
4. Target codegen should consume backend IR semantics without reparsing text.

## Execution Rules

- Prefer narrow, testable slices.
- Add or update targeted tests before each behavior change.
- Preserve compatibility shims where they reduce migration risk.
- If a separate initiative appears, record it under `ideas/open/` instead of
  widening this plan.

## Step 1: Establish Lowering Entrypoints

Goal: move the main production API away from adapter-first naming while keeping
behavior unchanged.

Primary target:
- [src/backend/lir_adapter.hpp](/workspaces/c4c/src/backend/lir_adapter.hpp)
- [src/backend/lir_adapter.cpp](/workspaces/c4c/src/backend/lir_adapter.cpp)

Actions:
- inspect the current `adapt_minimal_module()` call chain and all public entrypoints
- introduce a lowering-named entrypoint such as `lower_to_backend_ir(...)`
- keep adapter-named wrappers only as compatibility shims if needed
- update direct callers to use the lowering-oriented API where low-risk

Completion check:
- the main path exposes a lowering-oriented entrypoint
- tests stay green with no semantic drift

## Step 2: Separate Decode From IR Construction

Goal: stop one unit from acting as decoder, backend IR builder, and target
compatibility layer all at once.

Primary target:
- [src/backend/lir_adapter.cpp](/workspaces/c4c/src/backend/lir_adapter.cpp)
- potential new files under [src/backend/lowering](/workspaces/c4c/src/backend/lowering)

Actions:
- identify legacy text-decoding helpers for calls, memory, and GEP-like paths
- extract decode/canonicalize helpers into a lowering-focused location
- keep backend IR construction logic distinct from syntax decoding

Completion check:
- decode responsibilities live in a dedicated lowering-focused layer
- backend IR construction is easier to trace independently of text parsing

## Step 3: Tighten Backend IR Semantics

Goal: shift `src/backend/ir.hpp` toward structured backend-native semantics.

Primary target:
- [src/backend/ir.hpp](/workspaces/c4c/src/backend/ir.hpp)

Actions:
- inventory string-carried semantics that block backend-native modeling
- convert the highest-value surfaces into structured representations
- update validation and printing paths as required by those changes

Completion check:
- at least one meaningful class of backend semantics is no longer represented
  only as free-form text
- validation/printing remain coherent

## Step 4: Remove Codegen Parse Dependencies

Goal: keep target emitters from parsing backend-text fragments on demand.

Primary target:
- target emitters and helper users of `parse_backend_*`

Actions:
- identify current emitter-side `parse_backend_*` dependencies
- migrate one dependency cluster at a time onto structured backend IR access
- leave any remaining compatibility parsing only in the lowering layer

Completion check:
- at least one target-side parsing dependency is removed
- codegen reads backend IR semantics directly for the migrated slice

## Step 5: Validate And Preserve Boundaries

Goal: prove the refactor is behavior-preserving and leaves a clean base for the
later `backend_ir -> bir` scaffold.

Actions:
- keep targeted tests aligned with each slice
- run full regression checks after each landed slice
- update planning state with completed slices, next slice, and blockers

Completion check:
- full suite remains monotonic
- the active plan still matches the linked source idea
- the codebase has a clearer lowering boundary for the follow-on BIR work
