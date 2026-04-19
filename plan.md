# Shared Text Identity And Semantic Name Table Refactor

Status: Active
Source Idea: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Activated from: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md

## Purpose

Turn idea 64 into an execution runbook that moves shared text identity above
the frontend and establishes semantic name tables before CFG and phi ownership
work hardens new prepared contracts.

## Goal

Provide a shared text-id substrate plus semantic name tables such as
`BlockLabelId` and `FunctionNameId` so downstream prepared/backend work can use
typed symbolic identities instead of `std::string` or raw `TextId`.

## Core Rule

Do not ship new public prepared contracts keyed by `std::string` or raw
`TextId` when a semantic id layer is the real requirement.

## Read First

- `ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md`
- `ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md`
- `ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md`
- `src/frontend/string_id_table.hpp`
- `src/frontend/link_name_table.hpp`
- `src/backend/prealloc/prealloc.hpp`

## Scope

- move the shared text-id table out of the frontend layer
- define semantic name-table/id types for backend-visible symbolic domains
- prepare the identity substrate needed by idea 62 CFG ownership work and idea
  63 phi completeness work
- keep downstream contract migration focused on identity surfaces, not on
  finishing CFG or phi algorithms in this runbook

## Non-Goals

- completing CFG ownership generalization itself
- completing phi destruction or parallel-copy resolution itself
- mixing this refactor into unrelated emitter or instruction-selection work
- introducing one mega-id type that erases domain meaning

## Working Model

- a shared text table owns canonical byte storage
- semantic tables map shared text ids into domain-specific ids such as
  `BlockLabelId`, `FunctionNameId`, `ValueNameId`, and `SlotNameId`
- downstream prepared/backend structs carry semantic ids, not symbolic strings
  and not bare text-storage keys

## Execution Rules

- keep the layering direction clean: backend/shared code must not depend on a
  frontend-private text-id header
- prefer semantic id tables modeled after `LinkNameTable` over bare `TextId`
  plumbing
- update `todo.md`, not this file, for routine packet progress
- require `build -> narrow proof` for each code slice
- stop short of broad CFG/phi rewrites once the identity substrate and first
  consumer migrations are coherent

## Step 1: Extract The Shared Text Table Layer

Goal: move the text-id table into a shared layer that frontend and backend code
can both depend on without layering inversion.

Primary targets:

- `src/frontend/string_id_table.hpp`
- destination shared/support layer files

Actions:

- move or rehome the text-id table interface into a layer above the frontend
- update includes and namespaces so existing users still compile against the
  shared location
- keep the data structure append-only and compatible with existing canonical
  text storage expectations

Completion check:

- the text-id table no longer lives under `src/frontend/`, and both frontend
  and backend code can include the shared layer cleanly

## Step 2: Introduce Semantic Name Tables

Goal: define domain-specific symbolic id tables on top of shared text storage.

Primary targets:

- shared naming utilities adjacent to the extracted text-id layer
- `src/frontend/link_name_table.hpp`
- `src/backend/prealloc/prealloc.hpp`

Actions:

- define typed ids such as `FunctionNameId`, `BlockLabelId`, `ValueNameId`,
  and `SlotNameId` where justified
- introduce semantic tables that intern shared `TextId` values into those
  domain ids
- keep the design consistent with `LinkNameTable` rather than inventing a
  string wrapper without domain meaning

Completion check:

- the repo has semantic name-table infrastructure that can express backend and
  prepared symbolic identity without depending on raw strings or bare text ids

## Step 3: Migrate The First Prepared Identity Surfaces

Goal: land the first downstream users on semantic ids so idea 62 can build on
them instead of starting from string-shaped contracts.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- small, adjacent prepared/backend users needed to prove the new substrate

Actions:

- convert the minimal prepared symbolic surfaces needed by upcoming CFG work to
  semantic ids
- keep this step focused on identity contracts, not full CFG or phi algorithm
  completion
- document the follow-on expectation that idea 62 should consume `BlockLabelId`
  and related semantic ids directly

### Step 3.1: Move Prepared Control-Flow And Addressing Boundaries To Semantic Ids

Goal: keep the already-started prepared control-flow, join, x86 local-slot,
module, and stack-layout boundaries on typed function and block identities
instead of repeated raw spelling lookup.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/stack_layout.cpp`
- adjacent prepared/x86 lookup boundaries already carrying control-flow or
  addressing metadata

Completion check:

- the main prepared control-flow and addressing entry boundaries resolve
  `FunctionNameId` and `BlockLabelId` once and any remaining string-view paths
  in this area are compatibility shims rather than the primary contract

### Step 3.2: Migrate Remaining Prepared Lookup Helpers And Liveness Consumers

Goal: move the remaining lookup-heavy prepared helpers off raw value and block
spellings so typed ids stay authoritative inside the helper graph.

Primary targets:

- residual lookup helpers in `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/liveness.cpp`
- adjacent call sites that still enter those helpers through `std::string_view`
  names

Completion check:

- the remaining hot prepared lookup paths resolve typed ids once at the edge
  and carry `BlockLabelId` and `ValueNameId` internally, with string-view
  overloads reduced to compatibility wrappers where still needed

### Step 3.3: Confirm Idea 62 Starter Surfaces Are Clean

Goal: leave the first prepared/backend identity surfaces in a state that idea
62 can consume without introducing new string-keyed contracts.

Primary targets:

- prepared/backend helper surfaces that upcoming CFG ownership work will build
  on first

Completion check:

- the first prepared/backend identity surfaces use semantic ids cleanly enough
  that idea 62 can proceed without introducing new string-keyed contracts

## Step 4: Validate The Refactor

Goal: prove the shared identity refactor without bundling unrelated backend
algorithm changes.

Actions:

- require a fresh build for every accepted slice
- choose narrow proof that exercises both extracted shared-layer users and the
  migrated prepared identity surfaces
- broaden validation when include-path or shared naming changes affect several
  subsystems

Completion check:

- accepted identity-refactor slices have fresh proof and the repo builds
  cleanly against the shared text/name-table layer
