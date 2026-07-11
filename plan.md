# LIR To BIR Adapter Boundary Umbrella Runbook

Status: Active
Source Idea: ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md

## Purpose

Classify the current `LIR -> BIR` adapter boundary before more backend case
repair, then generate ordered follow-up ideas for behavior-preserving
interface cleanup.

## Goal

Produce durable boundary documentation under `docs/lir_bir_adapter_boundary/`
and ordered follow-up ideas under `ideas/open/` without changing implementation,
test expectations, runtime behavior, or backend harness policy.

## Core Rule

This is umbrella triage, not implementation. Do not claim progress through
source changes, expectation rewrites, unsupported-marker edits, allowlist
filtering, helper renames, or testcase-shaped shortcuts.

## Read First

- `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/module.hpp`
- `docs/bir_core_cleanup/`
- `docs/bir_prealloc_fusion/`
- `docs/rv64_gcc_torture_post_contract/`

## Current Targets

- Durable handoff documents under `docs/lir_bir_adapter_boundary/`.
- Current `LIR -> BIR` public and detail interface surfaces.
- Existing BIR cleanup and BIR/prealloc fusion documentation.
- Current RV64 GCC torture backend evidence, treated as transient input only
  unless summarized into durable docs.
- Ordered follow-up ideas under `ideas/open/` for the first adapter-boundary
  cleanup families.

## Non-Goals

- Do not edit implementation files, tests, expectations, unsupported markers,
  allowlists, runtime behavior, timeout policy, or default harness contracts.
- Do not change BIR lowering semantics, MIR output, object output, or prepared
  module shape.
- Do not move RV64, AArch64, or x86 target-specific facts into canonical BIR.
- Do not treat ignored `build/` artifacts as canonical lifecycle state.
- Do not rewrite `PreparedBirModule` or MIR consumers in this umbrella route.

## Working Model

The current adapter boundary exposes import context, legacy type text parsing,
structured layout fallback, initializer parsing, local/global address
provenance, call ABI computation, and compatibility maps through a broad detail
surface. The umbrella should separate those responsibilities by first owning
layer, compare the split against existing documentation and failure evidence,
and create smaller implementation ideas only after the boundary is classified.

## Execution Rules

- Write durable conclusions under `docs/lir_bir_adapter_boundary/`.
- Keep transient scan paths as cited inputs, not canonical state.
- Classify responsibilities by first owning layer before recommending follow-up
  work.
- Keep follow-up ideas behavior-preserving and ordered by dependency.
- Separate LIR import cleanup from canonical BIR semantics,
  prepared/prealloc publication, and MIR consumer changes.
- If the inventory shows a better split than the source idea's required
  follow-up families, explain that split in the handoff docs and generated
  ideas.

## Steps

### Step 1: Inventory The Adapter Boundary

Goal: Establish the current `LIR -> BIR` public and detail interface surface.

Actions:

- Inspect the public lowering entry and detail headers.
- Inventory the implementation files under `src/backend/bir/lir_to_bir/`.
- Identify exposed responsibilities, compatibility maps, and downstream
  dependencies without changing source.
- Record the evidence sources that will be summarized into durable docs.

Completion Check:

- The handoff docs can name the concrete files and responsibility groups that
  make up the current adapter boundary.

### Step 2: Classify Responsibilities By First Owning Layer

Goal: Separate adapter responsibilities from BIR core, prepared/prealloc, and
MIR consumer concerns.

Actions:

- Classify each exposed responsibility as LIR import, structured type/layout
  bridge, initializer bridge, memory/address provenance import, call ABI
  import, canonical BIR semantic model, or prepared/prealloc handoff.
- Identify route-local compatibility maps that must remain import-local or be
  hidden behind narrower adapter contracts.
- Compare the classification against `docs/bir_core_cleanup/`,
  `docs/bir_prealloc_fusion/`, and RV64 post-contract evidence.

Completion Check:

- The durable classification makes first ownership explicit and does not
  collapse unrelated layers into a generic BIR cleanup bucket.

### Step 3: Write The Handoff Documents

Goal: Create the durable umbrella output under
`docs/lir_bir_adapter_boundary/`.

Actions:

- Write an interface inventory for the current boundary.
- Write a responsibility classification using the Step 2 ownership model.
- Write an ordered follow-up plan that cites the same evidence sources.
- Clearly distinguish durable summaries from transient `build/` scan
  artifacts.

Completion Check:

- `docs/lir_bir_adapter_boundary/` contains an interface inventory,
  responsibility classification, and ordered follow-up plan that agree on
  source evidence and ownership terms.

### Step 4: Generate Ordered Follow-Up Ideas

Goal: Convert the umbrella findings into narrow source ideas for future
execution.

Actions:

- Generate follow-up ideas under `ideas/open/` for the required families unless
  the documented inventory proves a better split.
- For each follow-up, name owned files, first owning layer, behavior-preserving
  proof surface, downstream layers it must not edit, and reviewer reject
  signals.
- Order follow-ups by boundary clarity and blast-radius reduction.

Completion Check:

- The generated ideas are dependency-ordered and do not mix LIR import cleanup
  with BIR semantic changes, prepared/prealloc publication changes, or MIR
  consumer rewrites.

### Step 5: Closure Readiness Check

Goal: Decide whether the umbrella source idea is complete.

Actions:

- Verify the handoff docs satisfy the source idea acceptance criteria.
- Verify the generated follow-up ideas cover the required follow-up families or
  document why a different split is better.
- Prepare a closure note summarizing evidence used, docs written, ideas
  generated, ordering, and deferred responsibilities.

Completion Check:

- The source idea can be closed by lifecycle workflow, or the remaining
  umbrella gaps are explicit in `todo.md` for the next packet.
