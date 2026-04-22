# Extract X86 Codegen Subsystem To Markdown For Phoenix Rebuild

Status: Active
Source Idea: ideas/open/78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md
Activated from: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md

## Purpose

Start the Phoenix rebuild by extracting the full x86 codegen subsystem into a
complete per-file markdown artifact set that captures the real ownership,
dispatch, and contract surfaces without treating the live `.cpp` / `.hpp`
files as the design.

## Goal

Produce a durable extraction set under `docs/backend/x86_codegen_legacy/` with
one companion `.md` for every in-scope legacy
`src/backend/mir/x86/codegen/*.cpp` / `x86_codegen.hpp` file plus a
directory-level `index.md`.

## Core Rule

Do not redesign or implement during this runbook. Extract the current
subsystem honestly, keep only important contracts with short fenced `cpp`
blocks, preserve one-to-one source-to-markdown coverage, and classify special
cases instead of copying whole files into markdown.

## Read First

- `ideas/open/78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md`
- `ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md`
- `src/backend/mir/x86/codegen/`
- `docs/backend/x86_codegen_subsystem.md`

## Scope

- the full `src/backend/mir/x86/codegen/` subsystem and its current ownership
  split
- a companion extraction artifact for every in-scope legacy `.cpp` / `.hpp`
  file
- one directory-level index artifact that summarizes the subsystem and points
  at the full extraction set
- how `prepared_*.cpp` duplicates, bypasses, or partially reimplements seams
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
- keep each per-file artifact compressed and representative rather than
  exhaustive
- use the directory index to explain subsystem-level ownership and dependency
  direction across the per-file set
- explicitly classify special-case logic as `core lowering`, `optional fast
  path`, `legacy compatibility`, or `overfit to reject`

## Execution Rules

- prefer `boundary scan -> artifact map -> contract extraction ->
  responsibility map -> prepared-route divergence -> artifact review`
- keep representative code blocks short and only for important APIs or
  contracts
- note where prepared routes should have reused existing canonical seams
- keep the extraction set complete enough that reviewers can check coverage at
  a glance by filename

## Step 1: Establish Extraction Boundary And Artifact Map

Goal: define exactly which legacy source files are in scope and what markdown
artifact each one maps to.

Primary targets:

- `src/backend/mir/x86/codegen/`
- `docs/backend/x86_codegen_legacy/`

Actions:

- enumerate every in-scope `.cpp` / `.hpp` file under
  `src/backend/mir/x86/codegen/`
- define the one-to-one markdown companion path for each legacy source file
- define the directory-level `index.md` sections for ownership buckets,
  dependency direction, entry points, prepared-route divergence, and proof
  surfaces

Completion check:

- the artifact map is complete enough that later packets can fill the
  extraction set without re-deciding file coverage

## Step 2: Extract Canonical Entry Points, Contracts, And Lowering Families

Goal: capture the important APIs, dispatcher seams, and representative
contracts from the non-prepared portion of x86 codegen.

Primary targets:

- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `src/backend/mir/x86/codegen/mod.cpp`
- `src/backend/mir/x86/codegen/emit.cpp`
- `src/backend/mir/x86/codegen/asm_emitter.cpp`
- `src/backend/mir/x86/codegen/shared_call_support.cpp`
- `src/backend/mir/x86/codegen/calls.cpp`
- `src/backend/mir/x86/codegen/returns.cpp`
- `src/backend/mir/x86/codegen/memory.cpp`
- `src/backend/mir/x86/codegen/prologue.cpp`
- `src/backend/mir/x86/codegen/variadic.cpp`
- `src/backend/mir/x86/codegen/alu.cpp`
- `src/backend/mir/x86/codegen/atomics.cpp`
- `src/backend/mir/x86/codegen/cast_ops.cpp`
- `src/backend/mir/x86/codegen/comparison.cpp`
- `src/backend/mir/x86/codegen/f128.cpp`
- `src/backend/mir/x86/codegen/float_ops.cpp`
- `src/backend/mir/x86/codegen/globals.cpp`
- `src/backend/mir/x86/codegen/i128_ops.cpp`
- `src/backend/mir/x86/codegen/inline_asm.cpp`
- `src/backend/mir/x86/codegen/intrinsics.cpp`
- `src/backend/mir/x86/codegen/route_debug.cpp`

Actions:

- extract representative public or pseudo-public contracts for each owned
  family
- record which files appear to own dispatch, shared helpers, and concrete
  lowering
- keep only short fenced `cpp` blocks for important surfaces

Completion check:

- the extraction set captures the canonical subsystem surfaces another agent
  would need before evaluating prepared-route divergence

## Step 3: Extract Prepared-Route Divergence And Complete Per-File Coverage

Goal: capture how the prepared route interacts with, duplicates, or bypasses
the canonical seams and finish the full per-file extraction set.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_countdown_render.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_param_zero_render.cpp`
- `docs/backend/x86_codegen_legacy/index.md`

Actions:

- extract the major prepared-route responsibilities and hidden dependencies
- record where prepared helpers reimplement logic that should likely live in
  canonical seams
- classify representative special cases as core logic, optional fast path,
  legacy compatibility, or overfit to reject
- verify that every in-scope legacy source file has its markdown companion

Completion check:

- the extraction set makes the prepared-route divergence explicit and the
  per-file coverage is complete enough for stage 2 review

## Step 4: Validate Compression Quality And Handoff Readiness

Goal: finish a complete subsystem artifact set that is compressed, reviewable,
and ready to drive the next Phoenix stage.

Primary targets:

- `docs/backend/x86_codegen_legacy/`

Actions:

- remove file-dump style detail that does not carry contract or ownership
  meaning
- ensure the directory index points at every per-file artifact
- confirm the set still names the real ownership overlaps, false couplings,
  and available proof surfaces

Completion check:

- every in-scope legacy source file has a corresponding artifact under
  `docs/backend/x86_codegen_legacy/`, `index.md` summarizes the subsystem, and
  the set is compressed enough to serve as the stage-2 review target
