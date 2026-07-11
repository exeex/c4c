# LIR Import Context Extraction Runbook

Status: Active
Source Idea: ideas/open/685_lir_import_context_extraction.md
Activated from: LIR -> BIR adapter boundary first wave, order 1 of 6

## Purpose

Extract the public and private LIR-to-BIR import context into narrower
adapter-owned contracts while preserving all current lowering behavior.

## Goal

Make the adapter import context more explicit and private without changing BIR
output, diagnostics, notes, result-envelope behavior, unsupported markers,
tests, expectations, allowlists, object output, or runtime behavior.

## Core Rule

This is a behavior-preserving adapter-boundary cleanup. Do not claim progress
through semantic lowering repair, expectation changes, unsupported downgrades,
or testcase-shaped shortcuts.

## Read First

- `ideas/open/685_lir_import_context_extraction.md`
- `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`
- `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
- `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`

## Current Targets

- Public adapter entry:
  - `src/backend/bir/lir_to_bir.hpp`
  - `src/backend/bir/lir_adapter_error.hpp`
  - root `src/backend/bir/lir_to_bir.cpp`
- Split adapter implementation:
  - `src/backend/bir/lir_to_bir/analysis.cpp`
  - `src/backend/bir/lir_to_bir/context.cpp`
  - `src/backend/bir/lir_to_bir/module.cpp`
  - selected import-local declarations in
    `src/backend/bir/lir_to_bir/lowering.hpp`
- Adapter-local state families:
  - `ValueMap`
  - CFG and phi scratch maps
  - raw producer spelling maps
  - import diagnostics and notes
  - prescan facts
  - route-local orchestration state

## Non-Goals

- Do not edit public BIR route schemas, public BIR query surfaces, printer,
  validator, canonical semantic records, prepared/prealloc module shape,
  lookup bundles, frame/stack/call/storage products, MIR consumers, target
  emission, tests, expectations, unsupported markers, allowlists, runtime
  behavior, or harness policy.
- Do not combine this work with structured layout, initializer,
  memory/provenance, or call ABI semantic repair.
- Do not move import-local scratch state into public BIR, prepared/prealloc,
  target, or MIR ownership.

## Working Model

The public adapter entry and result envelope remain stable. The cleanup should
reduce the width of adapter-private declarations and make import-owned state
clearer without changing what the adapter accepts, rejects, reports, or emits.

`lowering.hpp` may still exist after this runbook, but any touched
declarations should become narrower, more local, or better grouped around a
real adapter-owned contract.

## Execution Rules

- Prefer small behavior-preserving steps that compile before continuing.
- Keep source-idea edits unnecessary unless durable intent changes.
- Record packet progress and proof in `todo.md`.
- If a packet discovers structured layout, initializer, memory/provenance, or
  call ABI work, stop and route that to the later ordered idea instead of
  expanding this plan.
- If a proposed change only renames helpers while preserving the same broad
  responsibility pile, reject it as insufficient progress.

## Ordered Steps

### Step 1: Establish Import Context Baseline

Goal: identify the current public entry behavior and private import-context
state before editing.

Primary targets:
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_adapter_error.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/analysis.cpp`
- `src/backend/bir/lir_to_bir/context.cpp`
- `src/backend/bir/lir_to_bir/module.cpp`

Actions:
- Read the handoff docs listed in `Read First`.
- Map public adapter entry behavior: options, result envelope, notes,
  diagnostics, optional BIR result, and throwing convenience entry.
- Inventory private state currently exposed through `lowering.hpp`, especially
  import diagnostics, notes, prescan facts, CFG/phi scratch maps, raw producer
  spelling maps, and route-local orchestration state.
- Choose one narrow first packet that reduces declaration width or clarifies
  ownership without crossing into later ordered ideas.

Completion check:
- The selected packet is recorded in `todo.md`, names its owned files, and
  explains why it is adapter-local and behavior-preserving.

### Step 2: Preserve Public Entry And Result Envelope

Goal: keep the public LIR-to-BIR adapter contract stable while private state is
narrowed behind it.

Primary targets:
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_adapter_error.hpp`
- `src/backend/bir/lir_to_bir.cpp`

Actions:
- Inspect the public entry and throwing convenience path before changing
  private implementation.
- If a private extraction touches public entry code, prove that notes,
  diagnostics, optional result behavior, and throwing behavior remain
  equivalent.
- Avoid public API churn unless it directly hides adapter-private state without
  changing behavior.

Completion check:
- Public entry declarations and behavior are unchanged or narrower for an
  explicit adapter-boundary reason, with build proof recorded in `todo.md`.

### Step 3: Narrow One Import-Local State Family

Goal: move one concrete import-context state family behind a smaller
adapter-owned contract.

Primary targets:
- selected declarations in `src/backend/bir/lir_to_bir/lowering.hpp`
- the split implementation file that already owns the chosen state family

Actions:
- Pick one family from the Step 1 inventory: diagnostics/notes, prescan facts,
  CFG/phi scratch maps, raw producer spelling maps, `ValueMap`, or route-local
  orchestration state.
- Prefer local ownership inside the existing implementation file when no other
  translation unit needs the declaration.
- Introduce a narrow private helper boundary only when it removes real
  cross-translation-unit coupling.
- Keep import-local maps and scratch state out of public BIR,
  prepared/prealloc, target, and MIR layers.

Completion check:
- The selected state family is narrower or more private than before, and the
  affected adapter translation units compile.

### Step 4: Contract Adjacent Private Declarations

Goal: reduce private declaration width created by the first extraction without
turning this into the broader order-2 header-contraction idea.

Primary targets:
- `src/backend/bir/lir_to_bir/lowering.hpp`
- adjacent private adapter implementation files touched by the packet

Actions:
- Remove declarations that became local after Step 3.
- Keep only declarations still needed across split adapter translation units.
- Avoid sweeping unrelated `lowering.hpp` cleanup that belongs to
  `ideas/open/686_private_detail_header_contraction.md`.

Completion check:
- The packet leaves `lowering.hpp` no broader than before and preferably
  narrower for the touched family, without absorbing order-2 scope.

### Step 5: Prove Behavior Preservation

Goal: validate the adapter-boundary cleanup with fresh focused proof.

Actions:
- Run a fresh build or compile proof that covers the touched adapter
  translation units.
- Run the focused BIR or LIR-to-BIR test target selected by the supervisor for
  the public lowering entry and result envelope.
- Escalate only if shared non-adapter surfaces were touched.

Completion check:
- `todo.md` records the exact proof command and result.
- No expectation, unsupported marker, allowlist, runtime behavior, or baseline
  policy changed.
