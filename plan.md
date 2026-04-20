# Prepared Value-Location And Move Consumption For X86

Status: Active
Source Idea: ideas/open/60_prepared_value_location_consumption.md
Activated from: ideas/closed/58_bir_cfg_and_join_materialization_for_x86.md

## Purpose

Turn idea 60 into an execution runbook that makes prepared value homes and
move obligations authoritative inputs to the x86 prepared emitter instead of
leaving x86 to reconstruct storage and ABI movement from regalloc internals,
slot order, or ad hoc conventions.

## Goal

Publish a consumer-oriented prepared value-location contract so x86 and nearby
downstream consumers can ask where a value lives and which moves are required
at joins, calls, and returns without reverse-engineering that meaning locally.

## Core Rule

Do not count one more x86 matcher or testcase-shaped home guess as progress.
New work must generalize the prepared handoff so value-home and move semantics
are shared, direct lookups.

## Read First

- `ideas/open/60_prepared_value_location_consumption.md`
- `ideas/closed/58_bir_cfg_and_join_materialization_for_x86.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/backend.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`

## Scope

- publish direct prepared lookups for value homes and grouped move obligations
- build the consumer view from existing regalloc and stack-layout outputs
- wire the new contract into the prepared-module handoff
- migrate x86 consumers away from emitter-local home reconstruction and ABI
  movement guessing

## Non-Goals

- stack-frame and addressing provenance work tracked by idea 61
- generic scalar instruction-selection work tracked by idea 59
- rebuilding branch/join semantics already owned by closed idea 58
- target-specific matcher growth that bypasses shared prepared ownership

## Working Model

- shared prepare already knows assigned registers, stack slots, and move
  resolution details through `PreparedRegalloc`
- x86 should consume a narrower, lookup-oriented value-location surface instead
  of reading regalloc arrays and rediscovering intent
- join, call, and return movement remain shared prepared facts even when x86 is
  the immediate consumer
- the prepared-module handoff stays authoritative: consumers ask the module for
  value homes and move bundles rather than inferring storage from local order

## Execution Rules

- keep value-home and move-bundle ownership in shared prepare, not x86-local
  fallback helpers
- preserve existing typed identity boundaries and prepared lookup conventions
- prefer `todo.md` for packet progress; rewrite this file only for route
  corrections
- require `build -> narrow backend proof` for each code slice that changes the
  contract or its x86 consumer
- broaden validation when a slice changes shared prepare plus multiple consumer
  families

## Step 1: Inventory Producer And Consumer Surfaces

Goal: map the exact prepared/regalloc producers and x86 consumers so the route
stays focused on publishing a consumer contract instead of growing new local
guesses.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`

Actions:

- identify where assigned registers, stack slots, and move-resolution records
  already exist in `PreparedRegalloc`
- identify the current x86 or adjacent consumer paths that still infer value
  homes or ABI movement indirectly
- record the narrow handoff helpers that should become canonical prepared
  lookups

Completion check:

- the route has a concrete inventory of producer data, missing consumer-facing
  lookups, and the x86 call/join/return surfaces that must migrate

## Step 2: Publish An Authoritative Prepared Value-Location Contract

Goal: add a consumer-oriented value-location model to shared prepare so
downstream code can query value homes and move obligations directly.

### Step 2.1: Add Prepared Value-Location Data And Lookup Helpers

Goal: define the public prepared structures and lookup surface that expose
value homes and grouped move obligations without leaking regalloc-private
details.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`

Actions:

- add the value-home, move-phase, move-bundle, and per-function container
  structures needed for direct consumer lookups
- add find/helper functions that match existing prepared lookup style
- keep the contract aligned with existing `PreparedBirModule` ownership rather
  than inventing x86-only parallel state

Completion check:

- shared headers expose a consumer-readable value-location contract and direct
  lookup helpers for function, value-home, and move-bundle queries

### Step 2.2: Build The Consumer View From Regalloc And Wire It Into The Prepared Module

Goal: materialize the new value-location contract from existing regalloc and
stack-layout outputs and publish it in the prepared-module handoff.

Primary targets:

- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/backend.cpp`

Actions:

- classify assigned register and stack-slot results into canonical value-home
  records
- group move-resolution output into stable join/call/return-oriented bundles
- store the resulting consumer view on `PreparedBirModule` without making x86
  re-read regalloc internals to recover the same meaning

Completion check:

- the prepared pipeline publishes populated value-location data on the module
  handoff using shared producer code

## Step 3: Move X86 Consumers To The Shared Value-Location Contract

Goal: make the x86 prepared route consume canonical value-home and move-bundle
lookups instead of rebuilding those answers locally.

### Step 3.1: Establish Minimal Move-Bundle Consumption For Scalar Home Queries

Goal: consume the bounded prepared move bundles required before minimal scalar
home lookups are authoritative.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- narrow backend proof surfaces under `tests/backend/`

Actions:

- identify the bounded single-block scalar routes where shared prepared homes
  still require `BeforeInstruction` or `BeforeReturn` moves before x86 can use
  them directly
- execute those prepared move bundles from shared move-plan data rather than
  keeping ABI/home fallbacks for the same routes
- keep the slice limited to the minimal scalar handoff family needed to make
  later value-home lookups authoritative; do not widen into the broader
  join/call/return surface yet

Completion check:

- the minimal scalar x86 handoff route consumes the required prepared
  `BeforeInstruction` and `BeforeReturn` bundles instead of assuming ABI-home
  placement at entry or return time

### Step 3.2: Replace Value-Home Guessing With Prepared Lookups

Goal: move operand and storage sourcing onto shared value-home queries once the
bounded prerequisite move execution exists.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- nearby x86 prepared helper surfaces if extraction is needed

Actions:

- replace emitter-local register/stack-home reconstruction with direct
  prepared-module lookups
- keep any remaining target logic limited to legality and spelling rather than
  storage discovery
- prove that stack-backed, register-backed, and rematerializable cases read the
  shared contract instead of local conventions

Completion check:

- x86 storage sourcing no longer depends on ad hoc regalloc-array inspection or
  local slot-order assumptions

#### Step 3.2.1: Finish The Remaining Bounded Scalar Home-Proof Lanes

Goal: close the remaining minimal scalar proof gaps so the prepared-home
consumer route is covered across the immediate-binary family before broader
cleanup.

Primary targets:

- `tests/backend/backend_x86_handoff_boundary_scalar_smoke_test.cpp`

Actions:

- finish the remaining bounded right-shift prepared-home lane coverage without
  widening the route beyond the minimal scalar family
- keep the proof work focused on authoritative prepared home consumption rather
  than target-local fallback helpers
- note whether any naturally produced stack-backed or rematerializable fixture
  can replace the current mutated-home probes while staying bounded

Completion check:

- the bounded scalar smoke route covers the remaining immediate-binary
  prepared-home gaps needed before broader Step 3.2 cleanup

#### Step 3.2.2: Prove A Naturally Produced Or Rematerializable Home Path

Goal: reduce dependence on mutated test-only prepared-home fixtures by proving
at least one bounded scalar route from a naturally produced stack-backed or
rematerializable home when the shared producer can supply it.

Primary targets:

- shared prepared producer surfaces if fixture support is missing
- `tests/backend/backend_x86_handoff_boundary_scalar_smoke_test.cpp`

Actions:

- confirm whether shared prepare already emits a naturally produced stack-backed
  or rematerializable scalar home for a bounded x86 handoff fixture
- if not, add only the minimal shared-producer support needed for one bounded
  proof lane
- keep the consumer contract shared and lookup-oriented rather than adding
  x86-only fixture shortcuts

Completion check:

- Step 3.2 has at least one bounded proof lane that reads a non-mutated shared
  prepared home or a shared rematerializable home

#### Step 3.2.3: Remove Residual Scalar Home-Reconstruction Seams

Goal: finish the minimal scalar consumer cleanup so x86 storage sourcing reads
prepared homes directly instead of reconstructing them from local conventions.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- nearby x86 prepared helper surfaces if extraction is needed

Actions:

- remove any remaining scalar-home reconstruction that still depends on local
  register or slot-order assumptions
- keep target logic limited to legality, spelling, and execution of already
  prepared moves
- prove the cleaned-up route still reads the shared prepared value-location
  contract for register-backed, stack-backed, and rematerializable-capable
  cases

Completion check:

- the minimal scalar x86 consumer no longer reconstructs storage that the
  prepared value-location contract already publishes

### Step 3.3: Consume Canonical Move Bundles For Join, Call, And Return Boundaries

Goal: move x86 boundary movement onto shared prepared move bundles.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- narrow backend proof surfaces under `tests/backend/`

Actions:

- consume grouped move bundles at the narrow join, call, and return boundaries
  that still need x86-local movement decisions
- keep target logic limited to executing the prepared move plan, not inferring
  which ABI or join moves should exist
- avoid widening into unrelated generic instruction-selection work

Completion check:

- x86 boundary movement reads prepared move bundles directly enough that join,
  call, and return handling no longer depends on local ABI/home guesswork

## Step 4: Validate The Prepared Value-Location Route

Goal: prove the new contract holds across the shared prepare handoff and its
bounded x86 consumer surfaces.

Primary targets:

- focused backend prepare/handoff tests covering prepared-module value-home and
  move-bundle behavior
- broader backend validation when shared prepare plus x86 consumer changes land

Actions:

- keep each packet on `build -> narrow backend proof`
- add or tighten proof for value-home lookup and boundary move-bundle
  consumption
- escalate to broader backend coverage before treating the runbook as ready to
  close

Completion check:

- the authoritative value-location handoff is covered by focused proof and any
  broader validation required by landed consumer-facing changes
