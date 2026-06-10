# Residual prepared lookup include cleanup runbook

Status: Active
Source Idea: ideas/open/149_residual_prepared_lookup_include_cleanup.md

## Purpose

Clean up residual `prepared_lookups.hpp` include pressure now that narrower
owner moves have landed.

Goal: make AArch64 and prealloc consumers include the narrowest available owner
headers, while keeping `prepared_lookups.hpp` only where a consumer still names
`PreparedFunctionLookups` or `make_prepared_function_lookups`.

## Core Rule

This is include cleanup only. Do not change lowering behavior, do not hide the
old prepared lookup facade behind a different broad umbrella header, and do not
remove `prepared_lookups.hpp` from a consumer that still needs aggregate
prepared lookup declarations.

## Read First

- `ideas/open/149_residual_prepared_lookup_include_cleanup.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- Current narrow owner headers for value-home, move-bundle, stack-layout,
  source-producer, publication, and same-block materialization declarations
- Current AArch64 and prealloc consumers that include `prepared_lookups.hpp`

## Current Targets And Scope

- AArch64 consumers identified by the idea:
  `dispatch_producers.cpp`, `dispatch_publication.cpp`,
  `dispatch_value_materialization.cpp`, `comparison.cpp`, `alu.cpp`,
  `memory.cpp`, `memory_store_retargeting.cpp`, `frame_slot_address.cpp`, and
  `fp_value_materialization.cpp` where relevant.
- Prealloc consumers identified by the idea:
  `formal_publications.*`, `decoded_home_storage.*`,
  `select_chain_lookups.cpp`, `publication_plans.cpp`, and
  `prepared_printer/select_chains.cpp` where relevant.
- Residual direct includes of `prepared_lookups.hpp` in nearby backend or
  prealloc files when the same classification proves they no longer need the
  facade.

## Non-Goals

- Do not perform declaration owner moves in this runbook.
- Do not remove `prepared_lookups.hpp` where the consumer still names
  `PreparedFunctionLookups` or `make_prepared_function_lookups`.
- Do not introduce an umbrella compatibility header to preserve broad include
  behavior under a new name.
- Do not change backend lowering semantics or test expectations.

## Working Model

Previous owner moves split declarations out of the prepared lookup facade. This
runbook audits the remaining consumers, classifies whether each include is
still required, and replaces stale broad includes with the narrow owner headers
that now export the declarations each consumer actually uses.

## Execution Rules

- Classify each target before editing it.
- Prefer the existing narrow owner header for each named declaration over
  transitive includes.
- Keep includes local to the consumer's actual declarations; do not add a broad
  umbrella just to reduce edit count.
- If a target still needs the prepared lookup aggregate or factory, leave its
  `prepared_lookups.hpp` include in place and record why in `todo.md`.
- Treat behavior changes, expectation downgrades, and named-case-only cleanup as
  route failures.

## Ordered Steps

### Step 1: Map residual prepared lookup includes and narrow owners

Goal: identify which consumers can drop `prepared_lookups.hpp` and which
narrow owner header each one should include instead.

Primary target: residual `prepared_lookups.hpp` consumers in the AArch64 and
prealloc target sets.

Actions:

- Use repository search to list every target consumer that includes
  `prepared_lookups.hpp`.
- For each target, identify whether it still names `PreparedFunctionLookups` or
  `make_prepared_function_lookups`.
- For removable broad includes, identify the narrow owner header that declares
  each used value-home, move-bundle, stack-layout, source-producer,
  publication, or same-block materialization API.
- Record the keep/remove classification in `todo.md` before broad edits.

Completion check:

- The executor can name the files to change, files to leave alone, and the
  narrow replacement headers for each removable `prepared_lookups.hpp` include.

### Step 2: Clean AArch64 residual includes

Goal: remove stale broad prepared lookup includes from AArch64 consumers.

Primary target: AArch64 files listed in the source idea and confirmed by Step
1.

Actions:

- Replace removable `prepared_lookups.hpp` includes with the narrow owner
  headers identified in Step 1.
- Preserve `prepared_lookups.hpp` in any AArch64 target that still requires the
  aggregate prepared lookup facade.
- Keep changes limited to includes and directly required compile fixes for
  declaration visibility.

Completion check:

- AArch64 targets compile without relying on `prepared_lookups.hpp` where Step
  1 classified the include as removable.

### Step 3: Clean prealloc residual includes

Goal: remove stale broad prepared lookup includes from prealloc consumers.

Primary target: prealloc files listed in the source idea and confirmed by Step
1.

Actions:

- Replace removable `prepared_lookups.hpp` includes with the narrow owner
  headers identified in Step 1.
- Preserve `prepared_lookups.hpp` in any prealloc target that still requires
  `PreparedFunctionLookups` or `make_prepared_function_lookups`.
- Avoid moving declarations or changing prepared lookup construction while
  performing include cleanup.

Completion check:

- Prealloc targets compile without relying on `prepared_lookups.hpp` where Step
  1 classified the include as removable.

### Step 4: Prove cleanup and remaining facade uses

Goal: validate that include cleanup is complete for the target scope and
behavior-preserving across backend coverage.

Actions:

- Re-run repository search for `prepared_lookups.hpp` in the target sets.
- For any remaining target include, verify the file still names
  `PreparedFunctionLookups` or `make_prepared_function_lookups`, or record the
  explicit blocker in `todo.md`.
- Run `cmake --build --preset default`.
- Run `ctest --test-dir build -R '^backend_' --output-on-failure`.

Completion check:

- Residual broad includes in target files are justified by actual prepared
  facade use, and the required build plus backend CTest proof are green.
