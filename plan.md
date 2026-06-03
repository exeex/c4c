# Prealloc Raw Global Address Identity Fallback Contract Runbook

Status: Active
Source Idea: ideas/open/105_prealloc_raw_global_address_identity_fallback_contract.md

## Purpose

Eliminate, constrain, or explicitly document raw global spelling fallback in
prealloc prepared address and materialization paths when structured
`LinkNameId` identity is absent.

## Goal

Classify every raw global spelling fallback and prove that ordinary lowered
global routes reach prealloc through structured identity.

## Core Rule

Raw display names must not silently become semantic global identity when BIR
can provide structured `LinkNameId` facts. Prealloc may own target address
preparation, relocation interpretation, and TLS handling, but source/global
identity authority must stay structured whenever available.

## Read First

- `ideas/open/105_prealloc_raw_global_address_identity_fallback_contract.md`
- `src/backend/prealloc/addressing.hpp`
- Prealloc coordinator global-address and materialization producers
- Existing backend tests for ordinary globals, global GEPs, and global pointer
  initializer/materialization routes

## Current Targets

- Raw-name fallback in prepared global address construction.
- Raw-name fallback in global materialization construction.
- Missing `LinkNameId` behavior for ordinary lowered globals.
- Structured identity proof for ordinary global loads/stores, global address
  materialization, and initializer or GEP-derived global addresses.

## Non-Goals

- Do not change final target relocation selection or TLS model lowering.
- Do not rework string constants, labels, or non-global symbol carriers unless
  they share the same missing-ID contract.
- Do not redesign broad global initializers.
- Do not rewrite unrelated pointer-carrier provenance paths.

## Working Model

BIR owns global identity, extent, initializer facts, TLS/constant/external
flags, and target-neutral materialization policy. Prealloc owns target address
preparation and relocation/TLS interpretation. The remaining gap is the narrow
compatibility path where prepared global address or materialization construction
falls back to raw symbol spelling when `LinkNameId` is missing.

## Execution Rules

- Keep inventory and classification notes in `todo.md`.
- Prefer structured `LinkNameId` identity whenever it exists.
- If missing `LinkNameId` is an ordinary lowering bug, fail closed or assert at
  the narrow boundary rather than recovering through raw spelling.
- If any fallback remains, name the compatibility class and prove it is not the
  primary ordinary-global identity path.
- For code-changing steps, run `cmake --build --preset default` before handing
  the packet back.
- Escalate validation if shared prepared addressing, global materialization, or
  initializer routes change.

## Step 1: Inventory Raw Global Identity Fallback Sites

Goal: trace every raw global spelling fallback in prepared address and
materialization construction.

Primary targets:

- `src/backend/prealloc/addressing.hpp`
- Prealloc coordinator global-address producers
- Global materialization and prepared-address construction helpers
- Existing backend global identity tests

Actions:

- Trace ordinary global load/store address construction.
- Trace global address materialization construction.
- Trace global GEP-derived address routes.
- Trace global pointer initializer routes.
- Identify every site that accepts raw spelling when `LinkNameId` is missing.
- Record whether each site currently behaves as ordinary identity authority,
  compatibility fallback, display-only fallback, or fail-closed input.

Completion check:

- `todo.md` lists each raw spelling fallback site, current behavior, and likely
  owner.
- Analysis proof confirms no implementation diff under `src/backend/bir` or
  `src/backend/prealloc`.

## Step 2: Decide The Missing-ID Contract

Goal: decide the status of every raw global spelling fallback.

Actions:

- Compare Step 1 findings against the source idea acceptance criteria.
- Classify each fallback as removed, assertion-worthy for ordinary globals, or
  retained as a named compatibility path.
- Decide proof targets for ordinary global loads/stores, global address
  materialization, and initializer or GEP-derived global address routes.
- Reject any route that makes raw spelling the primary identity path when
  structured IDs are available.

Completion check:

- `todo.md` records a concrete missing-ID contract, implementation targets,
  retained fallback names if any, and proof targets.
- Analysis proof confirms no implementation diff unless this packet
  intentionally begins implementation.

## Step 3: Implement The Structured Identity Boundary

Goal: enforce or document the chosen raw spelling fallback contract in prepared
address and materialization paths.

Actions:

- Prefer structured `LinkNameId` identity in global prepared address
  construction.
- Prefer structured `LinkNameId` identity in global materialization
  construction.
- Remove, assert, or constrain raw spelling fallback according to the Step 2
  contract.
- Preserve final target relocation and TLS policy behavior.
- Avoid touching unrelated pointer-carrier provenance or broad initializer
  design.

Completion check:

- The default build passes.
- Every remaining raw spelling fallback is named and narrow.
- Ordinary global identity cannot silently fall back to raw spelling when
  structured identity should exist.

## Step 4: Add Focused Global Identity Proof

Goal: prove structured identity for ordinary global, global materialization,
and initializer or GEP-derived routes.

Actions:

- Add or strengthen tests for ordinary global loads/stores through structured
  identity.
- Add or strengthen tests for global address materialization through structured
  identity.
- Add or strengthen tests for at least one initializer or GEP-derived global
  address route.
- Prefer prepared-address, prepared-plan, or contract assertions over fragile
  assembly-only symptoms when possible.

Completion check:

- The default build passes.
- Focused backend global identity tests pass.
- Tests do not weaken expectations, mark supported paths unsupported, or prove
  only one named global while leaving GEP/initializer routes unexamined.

## Step 5: Final Validation And Close Readiness

Goal: prove the completed route and prepare the source idea for closure review.

Actions:

- Run the default build and the relevant backend global/prepared-address subset.
- Include broader `^backend_` validation if shared prepared addressing, global
  materialization, or initializer interfaces changed.
- Update `todo.md` with final proof, retained fallback details if any, and
  close-readiness notes.

Completion check:

- Final delegated proof passes.
- `todo.md` shows coverage for ordinary global loads/stores, global address
  materialization, and initializer or GEP-derived global address routes.
- The source idea acceptance criteria are satisfied without relocation/TLS
  policy changes or unrelated pointer-carrier rewrites.
