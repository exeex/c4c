# Call-Bundle And Multi-Function Prepared-Module Consumption

Status: Active
Source Idea: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Supersedes: ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md

## Purpose

Turn idea 61 into an execution runbook that repairs prepared-module traversal
and prepared call-bundle consumption for x86 without reopening local ABI or
module-shape special cases in the emitter.

## Goal

Make owned prepared-module and prepared call-bundle failures, including the
newly graduated `c_testsuite_x86_backend_src_00040_c`, advance through generic
prepared x86 consumption instead of stopping at single-function or missing
call-bundle restrictions.

## Core Rule

Do not add another bounded `main + helper` lane or local ABI fallback in x86.
Prefer target-independent prepared contract publication or shared plan helpers,
then keep x86 rendering thin over that contract.

## Read First

- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_direct_extern_call_test.cpp`
- `tests/c/external/c-testsuite/src/00040.c`

## Scope

- generic prepared-module traversal across same-module multi-function cases
- authoritative prepared `BeforeCall` / `AfterCall` bundle consumption for x86
- shared prepared contract or helper growth when current prepared facts are not
  expressive enough for generic module or call-lane plans
- proof on owned backend and c-testsuite cases, not only handoff boundary tests

## Non-Goals

- reopening semantic lowering or stack/addressing work that already reaches the
  prepared-module handoff
- CFG/guard-chain ownership from idea 59
- scalar selection or terminator shaping unrelated to call lanes or
  multi-function module traversal
- runtime correctness bugs once codegen already succeeds

## Working Model

- the dominant failure is now prepared-module consumption, not missing semantic
  lowering
- `prepared_module_emit.cpp` currently rejects valid same-module inventory
  shapes too early instead of following a generic prepared traversal
- `x86_codegen.hpp` should consume authoritative `PreparedMoveBundle` metadata
  rather than inferring call setup or results locally
- if owned cases expose a real contract gap, the fix belongs in shared
  prepared structures and producers before x86 matching logic grows

## Execution Rules

- prefer one bounded prepared-module or call-lane seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow backend proof` for every accepted code slice
- include at least one owned c-testsuite case whenever a packet claims backend
  c-testsuite progress
- reject slices whose main effect is topology-specific x86 matching or local
  ABI fallback reopening

## Step 1: Audit Prepared-Module And Call-Bundle Boundaries

Goal: map the current prepared-module restriction and call-bundle failures to
the specific shared contract or traversal seams that need repair.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `src/backend/prealloc/prealloc.hpp`
- `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_direct_extern_call_test.cpp`
- `tests/c/external/c-testsuite/src/00040.c`

Actions:

- trace why `00040` now stops at the single-function prepared-module boundary
- inspect the existing bounded multi-defined-call and direct-extern-call
  boundary fixtures to see which prepared facts are already authoritative
- identify whether the first repair belongs to generic module traversal, call
  plan construction, or missing shared prepared contract fields

Completion check:

- the first implementation packet is narrowed to one concrete prepared-module
  or call-bundle seam with proof targets that cover both boundary tests and an
  owned c-testsuite case when relevant

## Step 2: Canonicalize Prepared-Module Traversal

Goal: let x86 consume supported same-module multi-function inventory without
hard-coding one bounded entry topology.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/prealloc/prealloc.hpp`

Actions:

- factor or extend a generic prepared-module traversal/classification helper
  over the prepared module inventory already published at handoff
- keep any new facts target-independent when the current handoff does not
  describe supported function relationships clearly enough
- reject emitter-local shortcuts that only accept one named module shape

Completion check:

- owned multi-function cases stop failing solely because x86 insists on a
  single-function or one special-case same-module topology

## Step 3: Consume Authoritative Prepared Call Bundles

Goal: make x86 follow prepared `BeforeCall` and `AfterCall` obligations as the
canonical call-lane contract.

Primary targets:

- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`

Actions:

- build or tighten a normalized call-lane plan from `PreparedMoveBundle` and
  `PreparedValueLocations` data
- extend shared prepared contract surfaces first if existing bundles do not
  publish enough durable call-lane meaning
- keep x86 rendering thin and do not reopen local ABI inference

Completion check:

- owned prepared call-bundle cases stop failing because the emitter consumes
  authoritative prepared call metadata generically

## Step 4: Validate Progress Against Owned Families

Goal: prove that accepted slices advance real prepared-module and call-bundle
capability instead of only moving boundary fixtures.

Actions:

- require a fresh build for every accepted code slice
- prove single-function restriction work with at least one owned c-testsuite
  case plus relevant backend handoff boundary coverage
- prove call-bundle work with the affected backend handoff boundary coverage
  plus an owned c-testsuite case when a c-testsuite claim is made
- broaden validation when a packet changes both module traversal and call-lane
  consumption or extends shared prepared contract fields

Completion check:

- accepted slices have fresh proof and show credible progress across owned
  prepared-module or call-bundle failures rather than a new bounded x86 lane
