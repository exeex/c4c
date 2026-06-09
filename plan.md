# Same-block load-local stored-value owner runbook

Status: Active
Source Idea: ideas/open/148_same_block_load_local_stored_value_owner.md

## Purpose

Move same-block load-local stored-value lookup toward the addressing or memory
owner without changing lowering behavior.

Goal: give `PreparedSameBlockLoadLocalStoredValueSource` and
`find_prepared_same_block_load_local_stored_value_source` a narrow
memory/addressing home that reuses prepared facts.

## Core Rule

Preserve semantic lookup behavior. Do not shape the move around one AArch64
call-site case, and do not duplicate stack-layout or same-block source-producer
scans when prepared facts already exist.

## Read First

- `ideas/open/148_same_block_load_local_stored_value_owner.md`
- The current declaration and definition of
  `PreparedSameBlockLoadLocalStoredValueSource`
- The current declaration and definition of
  `find_prepared_same_block_load_local_stored_value_source`
- `src/backend/prealloc/addressing.hpp`
- The existing addressing implementation owner selected by the current code

## Current Targets And Scope

- Move `PreparedSameBlockLoadLocalStoredValueSource`.
- Move `find_prepared_same_block_load_local_stored_value_source`.
- Use `src/backend/prealloc/addressing.hpp` and the existing addressing
  implementation owner selected by the codebase.
- Depend on the stack-layout owner from idea 143 and the same-block
  source-producer owner from idea 144.

## Non-Goals

- Do not rewrite prepared memory-access construction.
- Do not move AArch64 load/store emission, register view, scratch, or extension
  handling into prealloc.
- Do not special-case the current AArch64 call consumer.
- Do not absorb unrelated same-block materialization APIs into addressing.
- Do not weaken tests or mark supported backend paths unsupported.

## Working Model

The load-local stored-value lookup crosses source producers, memory-access
lookups, addressing, and stack layout. This runbook should narrow ownership by
moving only the load-local stored-value API and its finder into the
memory/addressing area while continuing to consume existing prepared source and
stack-layout facts.

## Execution Rules

- Keep each code step behavior-preserving unless an existing test exposes a
  true semantic bug in the moved API.
- Prefer moving declarations and definitions over adding forwarding facades.
- Include only the narrow headers needed by each consumer after the move.
- If shared memory-access semantics change, escalate from backend subset proof
  to full CTest.
- Treat testcase-shaped matching, named-case shortcuts, or expectation
  downgrades as route failure.

## Ordered Steps

### Step 1: Map the current API owner and prepared dependencies

Goal: identify the exact current owner, consumers, and prepared facts before
moving anything.

Primary target: current locations of
`PreparedSameBlockLoadLocalStoredValueSource` and
`find_prepared_same_block_load_local_stored_value_source`.

Actions:

- Locate the current declaration and definition of the struct and finder.
- Identify each direct consumer and include dependency.
- Identify which stack-layout and same-block source-producer prepared facts the
  finder should reuse after the move.
- Choose the existing addressing implementation file that should own the
  definition.

Completion check:

- The executor can name the old owner, new owner, direct consumers, and reused
  prepared facts before editing code.

### Step 2: Move the API into the addressing owner

Goal: relocate the load-local stored-value API without broadening addressing
ownership.

Primary target: `src/backend/prealloc/addressing.hpp` and the selected existing
addressing implementation owner.

Actions:

- Move `PreparedSameBlockLoadLocalStoredValueSource` into the addressing
  header.
- Move `find_prepared_same_block_load_local_stored_value_source` into the same
  owner family.
- Preserve the existing call contract and data shape unless the move exposes a
  clearly required narrow signature correction.
- Remove stale declarations from the previous owner.

Completion check:

- The moved API builds from the addressing owner and the previous owner no
  longer exports the load-local stored-value declarations.

### Step 3: Rewire consumers to the narrow owner

Goal: make consumers include and call the moved API through the new
memory/addressing home.

Primary target: direct consumers found in Step 1.

Actions:

- Replace old-owner includes with the narrow addressing or memory header where
  consumers need the moved API.
- Keep consumers that still require broader prepared lookup declarations on
  their existing includes.
- Ensure the finder reuses existing prepared stack-layout and same-block
  source-producer facts instead of rescanning.
- Avoid adding an umbrella compatibility include for the old owner.

Completion check:

- Direct consumers compile against the new owner, and no new duplicate
  source-producer or stack-layout scan is introduced for this lookup.

### Step 4: Prove the owner move

Goal: validate that the move is behavior-preserving across backend coverage.

Actions:

- Run `cmake --build --preset default`.
- Run `ctest --test-dir build -R '^backend_' --output-on-failure`.
- If the implementation changed shared memory-access semantics, also run full
  CTest with `ctest --test-dir build --output-on-failure`.

Completion check:

- Required build and backend subset proof are green, and any full CTest
  escalation required by semantic blast radius is also green.
