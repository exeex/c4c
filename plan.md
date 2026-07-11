# Initializer Lowering Bridge Isolation Runbook

Status: Active
Source Idea: ideas/open/688_initializer_lowering_bridge_isolation.md

## Purpose

Isolate global and aggregate initializer lowering as an adapter-owned bridge
from LIR spelling into semantic BIR facts without changing prepared object
data plans, relocation spelling, emitted data layout, tests, expectations, or
runtime behavior.

## Goal

Keep initializer compatibility and global declaration import state private to
the LIR-to-BIR adapter unless a later idea proves a stable public contract.

## Core Rule

This runbook is behavior-preserving. Do not change initializer semantics,
prepared object-data behavior, target data emission, diagnostics, object
output, runtime behavior, tests, expectations, unsupported markers, allowlists,
or harness policy.

## Read First

- ideas/open/688_initializer_lowering_bridge_isolation.md
- docs/lir_bir_adapter_boundary/ordered_followup_plan.md
- docs/lir_bir_adapter_boundary/interface_inventory_handoff.md
- docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md
- docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md
- docs/lir_bir_adapter_boundary/step2_responsibility_classification.md

## Current Targets

- src/backend/bir/lir_to_bir/globals.cpp
- src/backend/bir/lir_to_bir/global_initializers.cpp
- Root string constant rewrite helpers
- Initializer value materialization
- GlobalTypes
- FunctionSymbolSet
- Known global-address import paths

## Non-Goals

- Do not edit prepared object-data plans.
- Do not edit RV64 relocation spelling or target data emission.
- Do not change canonical BIR route schemas, MIR consumers, memory/provenance
  cleanup, call ABI cleanup, or prepared/prealloc ownership.
- Do not combine this work with semantic initializer repair.
- Do not change tests, expectations, unsupported markers, allowlists, runtime
  behavior, or harness policy.

## Working Model

Initializer lowering is an adapter bridge. It may translate LIR spelling,
global declarations, link names, string constants, scalar initializers, byte
strings, arrays, aggregates, pointer initializer offsets, and known global
addresses into stable BIR facts. It must not make textual initializer
compatibility part of public BIR, prepared object-data, prealloc, target, or
MIR ownership.

## Execution Rules

- Prefer small, mechanical slices that narrow visibility or clarify ownership.
- Preserve observable BIR output and downstream prepared/target behavior.
- Keep compatibility helpers import-local unless a later source idea expands
  the contract.
- Record packet progress and proof in todo.md, not in the source idea.
- Use compile proof for every code-changing step.
- Add global/initializer adapter coverage to the proof when touched paths
  affect initializer lowering behavior.
- If a slice needs prepared object-data or target emission changes, stop and
  ask the supervisor to split that downstream work into a separate idea.

## Ordered Steps

### Step 1: Inspect Initializer Bridge Surfaces

Goal: identify the current initializer bridge state and choose the first
behavior-preserving isolation seam.

Primary targets:
- src/backend/bir/lir_to_bir/globals.cpp
- src/backend/bir/lir_to_bir/global_initializers.cpp
- Root string constant rewrite helpers
- GlobalTypes
- FunctionSymbolSet
- Known global-address import paths

Actions:
- Inspect how global declarations, link-name resolution, string constants,
  initializer value materialization, and known global addresses enter BIR.
- Identify which data structures are already adapter-private and which expose
  textual compatibility or declaration import details too widely.
- Pick one narrow first packet that can reduce coupling without changing
  behavior.

Completion check:
- todo.md records the selected first packet, the exact files or helpers it
  owns, and the proof command the supervisor delegated.

### Step 2: Narrow Global Declaration Import State

Goal: keep global declaration and link-name compatibility state inside the
adapter boundary.

Primary targets:
- src/backend/bir/lir_to_bir/globals.cpp
- GlobalTypes
- FunctionSymbolSet

Actions:
- Extract or tighten helper contracts around global declaration import.
- Keep GlobalTypes and FunctionSymbolSet local to the import path unless a
  stable public contract already exists.
- Preserve emitted BIR facts and all downstream object-data behavior.

Completion check:
- The changed code builds.
- Existing global declaration behavior is unchanged under the supervisor's
  selected proof subset.
- todo.md records proof and any remaining coupling.

### Step 3: Isolate String Constant Rewrite State

Goal: make root string constant rewriting clearly adapter-owned.

Primary targets:
- Root string constant rewrite helpers
- src/backend/bir/lir_to_bir/globals.cpp
- src/backend/bir/lir_to_bir/global_initializers.cpp

Actions:
- Separate LIR spelling compatibility from semantic BIR publication.
- Keep rewrite bookkeeping private to initializer/global import.
- Do not change string data layout, relocation spelling, diagnostics, or
  emitted object contents.

Completion check:
- The changed code builds.
- The proof covers string constant or global initializer paths when touched.
- todo.md records proof and confirms no downstream data layout behavior
  changed.

### Step 4: Narrow Initializer Value Materialization

Goal: hide scalar, byte string, array, aggregate, and pointer initializer
spelling details behind adapter-owned materialization helpers.

Primary targets:
- src/backend/bir/lir_to_bir/global_initializers.cpp
- Initializer value materialization helpers

Actions:
- Clarify the boundary between textual initializer compatibility and semantic
  BIR initializer facts.
- Keep pointer initializer offsets and aggregate materialization details out of
  prepared object-data and target ownership.
- Avoid testcase-shaped initializer parsing or named-case-only shortcuts.

Completion check:
- The changed code builds.
- Existing initializer behavior is unchanged under global/initializer coverage.
- todo.md records proof and any follow-up that belongs in a separate idea.

### Step 5: Verify Known Global Address Import Boundaries

Goal: keep known global-address import state local to the adapter.

Primary targets:
- Known global-address import paths
- src/backend/bir/lir_to_bir/globals.cpp
- src/backend/bir/lir_to_bir/global_initializers.cpp

Actions:
- Inspect known global-address publication and consumers inside the adapter.
- Tighten helper signatures or ownership names only when that reduces real
  boundary leakage.
- Preserve public BIR records and downstream prepared/target behavior.

Completion check:
- The changed code builds.
- Proof covers the touched global-address or initializer paths.
- todo.md records whether any downstream cleanup must be split out.

### Step 6: Final Consistency And Proof Check

Goal: confirm the initializer bridge is narrower and behavior-preserving.

Actions:
- Re-read the source idea acceptance criteria and reviewer reject signals.
- Inspect the final diff for public BIR, prepared object-data, target data
  emission, test, expectation, unsupported-marker, allowlist, runtime, or
  harness changes.
- Run the supervisor-selected broader proof if previous packets touched more
  than one initializer/global surface.
- Record final proof and residual risks in todo.md.

Completion check:
- Initializer lowering compatibility state is narrower or more private within
  the adapter.
- GlobalTypes, FunctionSymbolSet, string constant rewrite state, initializer
  value materialization, and known global-address import remain import-local
  unless intentionally preserved behind a stable existing contract.
- Prepared object-data and target data emission behavior are unchanged.
- todo.md contains final proof for supervisor review.
