# AArch64 Dispatch Edge-Copy Helper Surface Privatization

Status: Active
Source Idea: ideas/open/131_aarch64_dispatch_edge_copy_helper_surface_privatization.md

## Purpose

Privatize or fold back the AArch64 dispatch edge-copy helper surface that is
currently public despite being limited to dispatch-family use.

## Goal

Narrow `dispatch_edge_copies.hpp` to real cross-file hooks, or remove it if no
public hook remains, while preserving edge-copy behavior.

## Core Rule

This is behavior-preserving ownership work. Do not change edge-copy ordering,
predecessor select parallel-copy behavior, block-entry publication behavior, or
prepared edge-copy/publication fact authority.

## Read First

- `ideas/open/131_aarch64_dispatch_edge_copy_helper_surface_privatization.md`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/CMakeLists.txt`
- closed audit note:
  `ideas/closed/130_aarch64_dispatch_family_post_contract_layout_audit.md`

## Scope

Primary files:

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`

Conditional file:

- `src/backend/CMakeLists.txt` only if a translation unit is removed

## Non-Goals

- Do not change emitted edge-copy instruction order.
- Do not change predecessor select parallel-copy lowering semantics.
- Do not change block-entry publication behavior.
- Do not replace prepared edge-copy or publication facts with local rediscovery.
- Do not move AArch64 register hazard policy or final instruction spelling into
  shared code.
- Do not use file-count reduction as the main justification.

## Working Model

Treat every public declaration in `dispatch_edge_copies.hpp` as suspect until
direct call-site inspection proves it is a real cross-file hook. Prefer the
smallest behavior-preserving contraction:

- make helpers file-local when only `dispatch_edge_copies.cpp` uses them;
- move helpers to `dispatch.cpp` only when dispatch orchestration is the clear
  owner;
- leave hooks public when cross-file call sites still require them;
- update build metadata only if the translation-unit set changes.

## Execution Rules

- Record routine progress in `todo.md`.
- Use call-site inspection before moving or deleting any declaration.
- Keep prepared fact lookups as consumers of existing prepared/shared facts.
- Preserve current tests and supported-path expectations.
- If public header or translation-unit shape changes, use matching
  `test_before.log` and `test_after.log`.

## Steps

### Step 1: Reconfirm Edge-Copy Public Surface

Goal: Identify every public declaration in `dispatch_edge_copies.hpp` and its
direct callers.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`

Actions:

- List every type and function declaration in `dispatch_edge_copies.hpp`.
- Map direct callers for each declaration.
- Classify each declaration as file-local, dispatch-orchestration-only, or real
  cross-file hook.
- Record any declarations that must remain public with a concrete caller-based
  rationale.

Completion check:

- `todo.md` contains a declaration/call-site table and a candidate action for
  each public edge-copy declaration.

### Step 2: Privatize File-Local Helpers

Goal: Remove header exposure for helpers that are only used inside
`dispatch_edge_copies.cpp`.

Primary targets:

- `dispatch_edge_copies.hpp`
- `dispatch_edge_copies.cpp`

Actions:

- Move file-local type or function declarations into
  `dispatch_edge_copies.cpp`.
- Keep names and behavior stable unless local scoping requires a minimal
  signature adjustment.
- Build after the contraction.

Completion check:

- File-local helpers no longer appear in the public header, and the default
  build succeeds.

### Step 3: Fold Dispatch-Orchestration Helpers If Clear

Goal: Move only clearly orchestration-owned helpers into `dispatch.cpp`, or
leave them public with rationale when movement would obscure ownership.

Primary targets:

- `dispatch_edge_copies.hpp`
- `dispatch_edge_copies.cpp`
- `dispatch.cpp`

Actions:

- For dispatch-orchestration-only helpers, decide whether `dispatch.cpp` is the
  clearer owner.
- Move only helpers whose ownership becomes simpler after the move.
- Avoid hidden cross-file dependencies or scattered forward declarations.
- If no helper should move, record the no-move rationale in `todo.md`.

Completion check:

- Any moved helper has a clearer owner and unchanged behavior, or `todo.md`
  records why movement was rejected.

### Step 4: Clean Header And Build Metadata If Needed

Goal: Leave a coherent edge-copy public surface and build file list.

Primary targets:

- `dispatch_edge_copies.hpp`
- `dispatch_edge_copies.cpp`
- `dispatch.cpp`
- `src/backend/CMakeLists.txt` only if a translation unit is removed

Actions:

- Remove stale includes, forward declarations, and public declarations.
- Delete the header or translation unit only if no real public hook or separate
  owner remains.
- Update `src/backend/CMakeLists.txt` only when a translation unit is removed.
- Check formatting around touched declarations and includes.

Completion check:

- The public edge-copy surface is minimal, coherent, and build metadata still
  matches the source set.

### Step 5: Validate Edge-Copy Behavior

Goal: Prove the contraction did not change supported AArch64 edge-copy
behavior.

Primary targets:

- default build
- AArch64 backend subset
- canonical regression logs if public header or translation-unit shape changed

Actions:

- Run `cmake --build --preset default`.
- Run `ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`.
- If public header or translation-unit shape changed, capture matching
  `test_before.log` and `test_after.log` and run the regression guard.
- Record proof results in `todo.md`.

Completion check:

- Build and AArch64 backend proof pass, with matching regression logs when the
  public/translation-unit shape changed.
