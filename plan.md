# BIR Route2 Select-Chain Body Extraction Runbook

Status: Active
Source Idea: ideas/open/523_bir_route2_select_chain_body_extraction.md

## Purpose

Clarify ownership of route2 select-chain and direct-global dependency analysis by moving implementation bodies out of `bir.cpp` while keeping the public route2 surface stable for route6 consumers.

## Goal

Extract existing route2 implementation bodies into `src/backend/bir/bir_route2.cpp` without changing direct-global dependency classification, route6 publication behavior, or public declarations in `bir.hpp`.

## Core Rule

This is a behavior-preserving body extraction. Do not change select-chain semantics, direct-global availability, route6 call-publication policy, call ABI behavior, tests, or expectations to make the move pass.

## Read First

- `ideas/open/523_bir_route2_select_chain_body_extraction.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir_private.hpp`
- `tests/backend/bir/CMakeLists.txt`
- `.codex/skills/c4c-clang-tools/SKILL.md` when mapping route2 symbols and consumers

## Current Targets

- Route2 select-chain producer and value-record bodies now in `src/backend/bir/bir.cpp`
- Direct-global dependency analysis used by route2 and consumed by route6 call-publication logic
- New body-only destination `src/backend/bir/bir_route2.cpp`
- Build metadata for `c4c_backend` and direct-source BIR tests that need the new TU

## Non-Goals

- Do not move public route2 declarations, route2 records, or route2 enums out of `bir.hpp`.
- Do not fold route2 helpers into route6 or expose private route2 internals to route6.
- Do not edit route6 publication policy, call ABI behavior, or idea 422 producer behavior.
- Do not split public headers in this slice.
- Do not weaken tests or rewrite expectations as proof of progress.

## Working Model

Route2 owns select-chain value analysis and direct-global dependency records. Route6 may continue to consume the public route2 surface, but it must not reach into route2-private implementation details. If extraction exposes a private helper dependency, introduce the narrowest BIR-private boundary needed instead of coupling route6 to route2 internals.

## Execution Rules

- Use AST-backed symbol queries from `c4c-clang-tools` when available; otherwise record the fallback command and reason in `todo.md`.
- Preserve all existing public names and declarations unless the source idea is explicitly updated by lifecycle review.
- Keep implementation moves mechanical and small enough to review.
- For code-changing steps, run `git diff --check`, build proof, and the focused backend tests delegated by the supervisor.
- Record proof commands, results, and any route6 consumer findings in `todo.md`.

## Steps

### Step 1: Audit route2 symbols and route6 consumers

Goal: identify the exact route2 bodies, private helpers, and route6 call-publication consumers before moving code.

Primary target: `src/backend/bir/bir.cpp`, `src/backend/bir/bir.hpp`

Actions:

- Map public route2 declarations in `bir.hpp`, including producer-kind, producer-record, value-record, value-index, and select-chain lookup entry points.
- Map route2 implementation bodies and any anonymous/private helpers in `bir.cpp`.
- Map route6 references to route2 direct-global dependency records and public route2 functions.
- Record whether any route2 helper needs a narrow BIR-private boundary.

Completion check:

- `todo.md` names the route2 body set, route6 consumer set, clang-tools availability, and safe extraction disposition.
- No implementation files are changed in this audit step.

### Step 2: Select extraction boundary

Goal: decide the exact body-only destination and helper boundary for implementation.

Primary target: `todo.md`

Actions:

- Confirm `src/backend/bir/bir_route2.cpp` as the destination if Step 1 finds only body-level route2 movement is required.
- Keep public route2 declarations and public record types in `bir.hpp`.
- Decide whether any helper must stay in `bir.cpp`, move with route2, or become a narrow declaration in `bir_private.hpp`.
- Reject any route that makes route6 depend on route2-private internals.

Completion check:

- `todo.md` records the selected move boundary, excluded symbols, build metadata implications, and unsafe alternatives.

### Step 3: Extract route2 bodies

Goal: move route2 implementation bodies into `src/backend/bir/bir_route2.cpp` with build metadata updated.

Primary target: `src/backend/bir/bir.cpp`, `src/backend/bir/bir_route2.cpp`, `tests/backend/bir/CMakeLists.txt`

Actions:

- Create `src/backend/bir/bir_route2.cpp` with the required includes and namespace structure.
- Move only the selected route2 bodies and route2-owned private helpers.
- Preserve public declarations in `bir.hpp`.
- Add private helper declarations to `bir_private.hpp` only if Step 2 selected that boundary.
- Update direct-source BIR test metadata if the new TU is needed outside `c4c_backend`.
- Run the delegated proof command exactly as provided by the supervisor.

Completion check:

- Build proof passes.
- Focused select-chain proof passes.
- Route6 call-publication coverage consuming route2 direct-global facts passes.
- `todo.md` records the command, result, and any proof log path.

### Step 4: Regression and review checkpoint

Goal: make the extraction acceptance-ready without relying on narrow proof alone.

Primary target: `todo.md`

Actions:

- Record the supervisor-selected broader regression or guard result.
- Confirm no direct-global select-chain availability changed.
- Confirm route6 still consumes the public route2 surface rather than private route2 implementation details.
- If a reviewer report is requested, absorb any required lifecycle correction into `todo.md` or `plan.md` before closure.

Completion check:

- `todo.md` records accepted narrow proof, broader validation, and any review disposition.

### Step 5: Close-readiness bookkeeping

Goal: prepare lifecycle state for supervisor acceptance and plan-owner close review.

Primary target: `todo.md`

Actions:

- Summarize moved bodies, preserved declarations, and unchanged route6/direct-global behavior.
- Record final proof status and whether `test_after.log` was rolled forward to `test_before.log`.
- List any leftover issues that should become separate open ideas instead of expanding this plan.

Completion check:

- `todo.md` states whether the runbook is complete and whether the source idea appears satisfied.
