# Extract X86 Codegen Subsystem To Markdown For Phoenix Rebuild

Status: Active
Source Idea: ideas/open/78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md
Activated from: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md

## Purpose

Start the Phoenix rebuild by extracting the current x86 codegen subsystem into
one compressed markdown artifact that captures the real ownership, dispatch,
and contract surfaces without treating the live `.cpp` files as the design.

## Goal

Produce `docs/backend/x86_codegen_subsystem.md` as the durable subsystem model
for later review and redesign.

## Core Rule

Do not redesign or implement during this runbook. Extract the current subsystem
honestly, keep only important contracts with short fenced `cpp` blocks, and
classify special cases instead of copying whole files into markdown.

## Read First

- `ideas/open/78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md`
- `ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `src/backend/mir/x86/codegen/mod.cpp`
- `src/backend/mir/x86/codegen/calls.cpp`
- `src/backend/mir/x86/codegen/returns.cpp`
- `src/backend/mir/x86/codegen/memory.cpp`
- `src/backend/mir/x86/codegen/prepared_countdown_render.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_param_zero_render.cpp`

## Scope

- the full `src/backend/mir/x86/codegen/` subsystem and its current ownership
  split
- how `prepared*.cpp` duplicates, bypasses, or partially reimplements seams
  that also exist in canonical x86 codegen files
- which helpers and contracts are truly shared versus renderer-local growth

## Non-Goals

- implementation edits under `src/backend/mir/x86/codegen/`
- drafting replacement interfaces
- choosing the final new file layout
- deleting legacy x86 codegen routes

## Working Model

- treat the current subsystem as executable evidence, not as the target design
- extract by responsibility bucket and dependency direction, not by file order
- prefer one subsystem artifact that points at representative surfaces over a
  markdown dump of every helper
- explicitly classify special-case logic as `core lowering`, `optional fast
  path`, `legacy compatibility`, or `overfit to reject`

## Execution Rules

- prefer `boundary scan -> contract extraction -> responsibility map ->
  special-case classification -> artifact review`
- keep representative code blocks short and only for important APIs or
  contracts
- note where prepared routes should have reused existing `calls.cpp`,
  `returns.cpp`, `memory.cpp`, or dispatcher seams
- name accidental ownership overlap explicitly in the artifact

## Step 1: Establish Extraction Boundary And Artifact Shape

Goal: define what the subsystem extraction must cover and what the markdown
artifact will contain.

Primary targets:

- `src/backend/mir/x86/codegen/`
- `docs/backend/x86_codegen_subsystem.md`

Actions:

- enumerate the current x86 codegen files and group them by responsibility
- define the artifact sections for entry points, shared contracts, ownership
  buckets, dependency directions, prepared-route divergence, and special-case
  classification
- keep the extraction boundary at the x86 codegen subsystem rather than
  expanding into unrelated backend layers

Completion check:

- the extraction target and markdown structure are concrete enough that the
  next packet can fill the artifact without re-deciding scope

## Step 2: Extract Stable Entry Points And Shared Contracts

Goal: capture the subsystem's important entry points, dispatcher seams, and
shared contracts from `x86_codegen.hpp` and adjacent canonical files.

Primary targets:

- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `src/backend/mir/x86/codegen/mod.cpp`
- `src/backend/mir/x86/codegen/emit.cpp`
- `src/backend/mir/x86/codegen/calls.cpp`
- `src/backend/mir/x86/codegen/returns.cpp`
- `src/backend/mir/x86/codegen/memory.cpp`

Actions:

- extract the representative APIs and helper contracts that appear to define
  canonical x86 codegen seams
- record which files appear to own dispatch versus concrete lowering
- keep only short fenced `cpp` blocks for the key contracts

Completion check:

- the artifact captures the stable subsystem surfaces another agent would need
  before evaluating prepared-route duplication

## Step 3: Extract Prepared-Route Divergence And Responsibility Mixing

Goal: capture how `prepared*.cpp` currently interacts with, duplicates, or
bypasses the canonical x86 codegen seams.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_countdown_render.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_param_zero_render.cpp`

Actions:

- extract the major prepared-route responsibilities and hidden dependencies
- record where prepared helpers reimplement logic that should likely live in
  canonical x86 codegen seams
- classify representative special cases as core logic, optional fast path,
  legacy compatibility, or overfit to reject

Completion check:

- the artifact makes the prepared-route divergence explicit enough for stage 2
  review to critique the subsystem design instead of rediscovering it

## Step 4: Validate Compression Quality And Handoff Readiness

Goal: finish a subsystem artifact that is compressed, reviewable, and ready to
drive the next Phoenix stage.

Primary targets:

- `docs/backend/x86_codegen_subsystem.md`

Actions:

- remove file-dump style detail that does not carry contract or ownership
  meaning
- ensure the artifact names the real ownership overlaps and false couplings
- confirm the artifact still points to representative code and proof surfaces

Completion check:

- `docs/backend/x86_codegen_subsystem.md` exists, is compressed correctly, and
  can serve as the stage-2 review target
