# Call-Argument Materialization Call Owner Runbook

Status: Active
Source Idea: ideas/open/146_call_argument_materialization_call_owner.md

## Purpose

Move call-argument source-producer materialization policy out of residual
prepared lookup ownership and into the call prealloc boundary.

## Goal

Make call-argument materialization declarations call-owned while reusing the
same-block/source-producer materialization owner from the earlier route.

## Core Rule

Keep this as an ownership move over existing semantic facts. Do not replace
prepared facts with AArch64-only shortcuts or case-shaped matching.

## Read First

- `ideas/open/146_call_argument_materialization_call_owner.md`
- `src/backend/prealloc/calls.hpp`
- current declarations and definitions for:
  - `PreparedCallArgumentSourceProducerMaterialization`
  - `prepared_call_argument_binary_producer_opcode_is_materializable`
  - `find_prepared_call_argument_source_producer_materialization`

## Scope

- Move call-argument materialization declarations to
  `src/backend/prealloc/calls.hpp`.
- Move or reconnect the matching implementation under the repo-local call
  owner pattern.
- Depend on the same-block/source-producer materialization owner from idea 144.
- Update consumers to include the call owner where they name call-argument
  materialization APIs.

## Non-Goals

- Do not move general same-block scalar producer helpers into call ownership.
- Do not change call-plan construction or ABI lowering.
- Do not move AArch64 call emission, register spelling, or scratch policy into
  prealloc.
- Do not mix ABI move-bundle ownership into this route.
- Do not weaken tests or rewrite expectations to claim ownership progress.

## Working Model

Call-argument materialization is call-specific policy over same-block source
producers. The call owner should expose the call-facing API and delegate shared
same-block/source-producer reasoning to the existing shared owner instead of
duplicating scans.

## Execution Rules

- Preserve behavior while moving ownership.
- Keep public declarations in the narrowest semantic owner.
- Remove stale `prepared_lookups.hpp` dependencies only where the consumer no
  longer needs prepared lookup aggregate APIs.
- If the move exposes broader include cleanup, record it in `todo.md` unless it
  is required for this ownership move.
- Run build proof after every code-changing packet and the backend CTest subset
  before acceptance.

## Step 1: Map Current Call-Argument Materialization Ownership

Goal: identify the exact declarations, definitions, and consumers that belong
to this route.

Concrete actions:

- Locate the current declaration and definition sites for:
  - `PreparedCallArgumentSourceProducerMaterialization`
  - `prepared_call_argument_binary_producer_opcode_is_materializable`
  - `find_prepared_call_argument_source_producer_materialization`
- Locate consumers that name those APIs directly.
- Identify the same-block/source-producer materialization owner dependency from
  the completed idea 144 work.
- Confirm whether the existing call implementation owner is already the right
  destination or whether a small companion implementation file is needed.

Completion check:

- The executor can name the source file changes needed for the move without
  expanding into ABI move bundles, AArch64 call emission, or generic same-block
  scalar producer ownership.

## Step 2: Move Public Call-Argument Materialization Declarations

Goal: make `src/backend/prealloc/calls.hpp` the public owner for call-argument
materialization APIs.

Concrete actions:

- Move `PreparedCallArgumentSourceProducerMaterialization` into
  `src/backend/prealloc/calls.hpp`.
- Move declarations for
  `prepared_call_argument_binary_producer_opcode_is_materializable` and
  `find_prepared_call_argument_source_producer_materialization` into the call
  owner.
- Include or forward-declare only the same-block/source-producer types needed
  by the moved declarations.
- Remove the declarations from residual prepared lookup ownership once the call
  owner exposes them.

Completion check:

- Call-argument materialization declarations are available through
  `src/backend/prealloc/calls.hpp`, and the old prepared lookup header no
  longer owns those declarations.

## Step 3: Move Definitions and Reconnect Shared Dependencies

Goal: place implementation under call ownership while preserving shared
same-block/source-producer behavior.

Concrete actions:

- Move the matching function definitions to the repo-local call owner
  implementation location.
- Reuse the same-block/source-producer materialization owner from idea 144 for
  shared materialization decisions.
- Keep call-specific filtering and call-argument policy in the call owner.
- Avoid copying same-block scans or producer classification logic into the call
  owner.

Completion check:

- Definitions compile from the call owner and still use shared semantic facts
  for same-block/source-producer materialization.

## Step 4: Update Consumers and Includes

Goal: route call-argument materialization users through the call owner without
starting broad residual include cleanup.

Concrete actions:

- Update direct consumers of the moved APIs to include
  `src/backend/prealloc/calls.hpp` or the repo-local relative form.
- Remove `prepared_lookups.hpp` includes only when they were needed solely for
  the moved call-argument materialization APIs.
- Leave consumers that still name `PreparedFunctionLookups`,
  `make_prepared_function_lookups`, or unrelated prepared lookup APIs on their
  existing include path.

Completion check:

- Consumers of the moved call-argument materialization APIs build through the
  call owner, and no broad include-cleanup route is mixed into this step.

## Step 5: Validate and Hand Back

Goal: prove the ownership move without claiming unrelated backend progress.

Concrete actions:

- Run `cmake --build --preset default`.
- Run `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Record proof results and any residual include-cleanup follow-up in `todo.md`.

Completion check:

- The build and backend subset pass, or failures are recorded with concrete
  ownership blockers for supervisor routing.
