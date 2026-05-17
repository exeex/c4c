# Backend and AArch64 Codegen Entrypoint Clarity Runbook

Status: Active
Source Idea: ideas/open/264_backend_and_aarch64_codegen_entrypoint_clarity.md

## Purpose

Make the backend driver and AArch64 codegen route easy to follow without
changing lowering semantics, emitted assembly, diagnostics, or test contracts.

## Goal

`src/backend/backend.cpp` should read as a high-level backend flow, while
AArch64-owned files expose the public compile entry and own target-specific
assembly rendering.

## Core Rule

This is behavior-preserving ownership and naming work. Do not expand AArch64
lowering capability, rewrite common MIR infrastructure, or route callers around
`c4c::backend::aarch64::codegen`.

## Read First

- `ideas/open/264_backend_and_aarch64_codegen_entrypoint_clarity.md`
- `ideas/closed/263_aarch64_codegen_public_compiled_module_interface.md`
- `src/backend/backend.cpp`
- `src/backend/mir/aarch64/codegen/codegen.hpp`
- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

## Current Scope

- Thin `src/backend/backend.cpp` to target selection, BIR/prepared handoff,
  route-debug selection, and target-owned public API calls.
- Move AArch64 `.s` text rendering details out of `backend.cpp` into an
  AArch64-owned helper under `src/backend/mir/aarch64/`.
- Keep the public AArch64 entry visible through
  `src/backend/mir/aarch64/codegen/codegen.hpp` and
  `compile_prepared_module(...)`.
- Rename or wrap misleading internal codegen entry files only where it makes
  the prepared-BIR-to-MIR route easier to trace.
- Update focused docs, tests, or contract text that describe the backend route.

## Non-Goals

- Do not change instruction selection, lowering semantics, diagnostics, or
  emitted assembly.
- Do not replace the shared MIR container or shared MIR printer boundary.
- Do not implement a native assembler or object writer.
- Do not move every AArch64 helper out of `module.hpp` for namespace cleanup.
- Do not do broad file shuffling that fails to reduce `backend.cpp`
  responsibility or clarify the public codegen entry.
- Do not rewrite tests or expectations to make a behavior change appear
  behavior-preserving.

## Working Model

The intended top-level flow is:

1. `backend.cpp` resolves target/profile/input route and prepares BIR as needed.
2. AArch64 public codegen compiles prepared BIR into a compiled MIR module.
3. A printer or future assembler consumer walks the compiled module.
4. Lowering internals remain named by responsibility:
   module compile coordinator, traversal, dispatch, and family lowerers.

## Execution Rules

- Keep each implementation step small enough to prove with build plus focused
  backend/AArch64 tests.
- Preserve the existing `.s` output route through shared MIR printing and
  AArch64 target instruction/operand printing.
- Prefer wrappers or file renames only when they improve call-graph
  discoverability for reviewers.
- Treat test expectation downgrades, named-case shortcuts, or semantic changes
  as route failures.
- Keep routine packet progress in `todo.md`; rewrite this runbook only for a
  real route correction.

## Ordered Steps

### Step 1: Audit Backend And AArch64 Route Responsibilities

Goal: classify current backend and AArch64 codegen ownership before moving
code.

Primary targets:

- `src/backend/backend.cpp`
- `src/backend/mir/aarch64/codegen/codegen.hpp`
- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Concrete actions:

- Classify each relevant `backend.cpp` helper as generic driver, x86 route,
  AArch64 route, route-debug filtering, shared BIR preparation, or AArch64
  assembly rendering.
- Identify the exact AArch64 machine-node-to-assembly rendering code that
  should leave `backend.cpp`.
- Confirm that public callers can continue entering AArch64 through
  `codegen::compile_prepared_module(...)`.
- Record any necessary packet notes in `todo.md` rather than editing the source
  idea.

Completion check:

- A later executor can name the AArch64 rendering helper boundary and the
  backend-driver code that should remain in `backend.cpp`.

### Step 2: Extract AArch64 Assembly Rendering Ownership

Goal: move AArch64 `.s` text rendering details from `backend.cpp` into an
AArch64-owned helper.

Primary targets:

- `src/backend/backend.cpp`
- `src/backend/mir/aarch64/`
- `src/backend/mir/aarch64/codegen/machine_printer.*`

Concrete actions:

- Add or reuse an AArch64-owned helper that renders the compiled module to
  assembly text for the current external assembler route.
- Keep `backend.cpp` responsible for selecting the route and calling the
  target-owned helper, not for constructing target printer details itself.
- Preserve shared MIR printer usage and AArch64-specific instruction/operand
  printing behavior.

Completion check:

- `backend.cpp` no longer owns AArch64 machine-node assembly rendering details,
  and focused backend smoke proof shows `.s` output is unchanged.

### Step 3: Clarify The Internal AArch64 Compile Coordinator

Goal: make the public compile entry and internal prepared-module coordinator
discoverable from file and function names.

Primary targets:

- `src/backend/mir/aarch64/codegen/codegen.hpp`
- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/emit.hpp`
- build files that list AArch64 codegen sources

Concrete actions:

- Rename, wrap, or split `emit.cpp` only if the resulting names better express
  the prepared-module compile coordinator role.
- Keep `codegen.hpp` as the public API surface for
  `compile_prepared_module(...)`.
- Update includes and build lists without changing codegen behavior.

Completion check:

- A reviewer can identify the public AArch64 entry and the internal module
  compile coordinator without reading unrelated lowering helpers.

### Step 4: Align Route Documentation And Contracts

Goal: make docs and contract tests describe the clarified route.

Primary targets:

- `src/backend/mir/aarch64/codegen/README.md`
- focused backend/AArch64 route docs or tests that mention the public entry
- signature or metadata contract tests for `codegen.hpp`

Concrete actions:

- Update route text to describe prepared BIR -> compiled MIR module -> shared
  MIR printer or future assembler consumer.
- Keep tests focused on ownership and API contracts, not expected-output
  rewrites.
- Avoid creating docs that imply `.s` text strings are the only reusable
  AArch64 codegen product.

Completion check:

- Route documentation and contract checks match the clarified ownership model.

### Step 5: Validate And Prepare Close Review

Goal: prove the active idea is complete without accepting semantic drift.

Concrete actions:

- Run the supervisor-delegated build and focused backend/AArch64 proof.
- Escalate to broader backend or full CTest validation if multiple code-moving
  packets have landed or the blast radius extends beyond one narrow route.
- Verify `backend.cpp`, `codegen.hpp`, internal coordinator naming, and docs
  satisfy the source idea completion criteria.

Completion check:

- Fresh proof is green or host-tool skips are explicitly reported, and no
  reviewer reject signal from the source idea applies.
