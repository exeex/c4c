# Plan 137: Select-chain public owner cleanup

Status: Active
Source Idea: ideas/open/137_select_chain_public_owner_cleanup.md

## Purpose

Give select-chain dependency queries one public owner and one include boundary without changing the prepared facts they compute.

## Goal

Move the select-chain lookup API out of split facade ownership so future backend code can discover select-chain dependency facts from a stable domain boundary.

## Core Rule

This is an API ownership cleanup. Do not change select-chain dependency semantics, result fields, AArch64 materialization behavior, store lowering, call lowering, or producer dispatch.

## Read First

- `ideas/open/137_select_chain_public_owner_cleanup.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/select_chain_lookups.cpp`
- Known consumers named in the source idea before changing includes.

## Current Targets

- Public declarations for:
  - `PreparedSelectChainDependencyQuery`
  - `find_prepared_direct_global_select_chain_dependency`
  - `find_prepared_select_chain_source_producer`
  - `find_prepared_store_source_direct_global_select_chain_dependency`
  - `find_prepared_scalar_select_chain_materialization`
- Include ownership in:
  - `src/backend/prealloc/prepared_lookups.hpp`
  - `src/backend/prealloc/publication_plans.hpp`
  - `src/backend/prealloc/select_chain_lookups.cpp`
- Consumers listed by the source idea, especially AArch64 codegen and prepared-printer users.

## Non-Goals

- Do not rewrite AArch64 select materialization, store lowering, call lowering, or producer dispatch.
- Do not delete shared select-chain facts or make consumers rescan BIR producers.
- Do not move store-source publication planning or call-argument planning into the select-chain module.
- Do not weaken tests, mark backend cases unsupported, or use printer-output rewrites as proof.

## Working Model

`select_chain_lookups.cpp` already houses the definitions. The cleanup should make the public declarations match that ownership, either by introducing a small paired select-chain lookup header or by documenting and enforcing a single existing domain owner if a new header creates unnecessary dependency churn.

## Execution Rules

- Prefer mechanical declaration/include movement over behavioral changes.
- Keep names target-neutral and semantic: direct-global dependency, source producer, and scalar materialization facts should not become AArch64-specific.
- Keep AArch64 consumers on prepared select-chain queries; do not replace query usage with local source-producer scans.
- If an include cycle appears, resolve it at the declaration boundary instead of broadening the cleanup into publication-plan or call-plan redesign.
- Use narrow build proof after each code-changing step; escalate validation only if select-chain fact semantics change.

## Ordered Steps

### Step 1: Map current ownership and consumers

Goal: Identify the exact declaration, definition, and include graph before moving any public API.

Actions:
- Inspect the declarations in `prepared_lookups.hpp` and `publication_plans.hpp`.
- Inspect definitions in `select_chain_lookups.cpp`.
- Find all current consumers of the five target names.
- Decide whether the stable owner should be a new paired select-chain lookup header or an existing documented domain boundary.

Completion Check:
- The next edit has one chosen owner and a complete consumer list, with no planned semantic changes.

### Step 2: Move declarations to the selected owner

Goal: Make select-chain query declarations available from one stable public boundary.

Actions:
- Add or adjust the selected owner header for the five target declarations.
- Remove duplicate or misplaced public exposure from `prepared_lookups.hpp` and `publication_plans.hpp`.
- Keep required forward declarations and includes minimal.
- Keep `select_chain_lookups.cpp` paired with the selected public owner.

Completion Check:
- The target declarations no longer have split facade ownership, and the implementation still compiles far enough to reveal consumer include fixes rather than semantic failures.

### Step 3: Update consumers to include the owner boundary

Goal: Make known consumers compile against the selected select-chain owner.

Actions:
- Update prealloc, prepared-printer, and AArch64 consumers named by the source idea.
- Remove accidental transitive dependency on `prepared_lookups.hpp` or `publication_plans.hpp` for select-chain query access.
- Do not introduce local producer scans, name matching, or target-local replacement logic.

Completion Check:
- Known consumers use the selected owner boundary directly or through a justified aggregate header, and no local rediscovery logic was added.

### Step 4: Validate and tighten the ownership boundary

Goal: Prove the cleanup is mechanical/API-oriented and did not alter select-chain facts.

Actions:
- Run `cmake --build --preset default`.
- Run `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Escalate to `ctest --test-dir build -j --output-on-failure` only if select-chain fact semantics changed.
- Inspect the diff for forbidden result-field changes and testcase-overfit signals.

Completion Check:
- Build and backend subset proof are fresh, or any blocker is recorded in `todo.md` with the exact failing command and failure.
