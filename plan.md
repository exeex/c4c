# Large Selected Pointer-Offset Local-Memory Policy Runbook

Status: Active
Source Idea: ideas/open/634_large_selected_pointer_offset_local_memory_policy.md

## Purpose

Define the RV64 selected pointer base-plus-offset local-memory policy for
accesses whose offsets exceed the narrow immediate load/store form.

## Goal

Move at least one complete-authority large selected pointer-offset
local-memory row past its current owner, or close the route with a precise
missing-authority or target-policy diagnosis.

## Core Rule

RV64 may materialize a large selected pointer offset only when explicit facts
identify the selected base, offset, width, address space, memory-use
freshness, scratch-register authority, clobber safety, and supported offset
range. Do not infer those facts from source filenames, final assembly shape,
specific constant offsets, or generic frame-slot local-memory behavior.

## Read First

- `ideas/open/634_large_selected_pointer_offset_local_memory_policy.md`
- `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
- `ideas/closed/599_pointer_base_plus_offset_selected_authority.md`
- `ideas/closed/600_pointer_value_memory_use_freshness_authority.md`
- RV64 local-memory consumers in
  `src/backend/mir/riscv/codegen/object_emission.cpp` and adjacent selected
  base-plus-offset emission helpers.
- Prepared selected pointer, memory-use freshness, access-width, and offset
  range carriers under `src/backend/prealloc/` and `src/backend/bir/`.

## Current Targets

- Large selected pointer-offset local-memory rows represented by
  `src/ipa-sra-2.c` and `src/pr60822.c`.
- Selected pointer base-plus-offset facts for base identity, selected offset,
  access width, address space, memory-use freshness, and supported range.
- RV64 target-policy facts for scratch-register availability, scratch clobber
  safety, materialization sequence ownership, and fail-closed diagnostics.

## Non-Goals

- Producing selected pointer base-plus-offset authority already owned by idea
  `599`.
- Producing pointer value memory-use freshness authority already owned by idea
  `600`.
- Generic frame-slot local-memory support already closed by idea `614`.
- Direct pointer arithmetic without selected authority.
- String-constant, direct global-symbol, aggregate stack-home, F128/16-byte
  width, ABI, runtime, expectations, unsupported markers, allowlists,
  timeouts, or accounting work.

## Working Model

- Evidence comes first. Refresh the large selected pointer-offset rows before
  choosing a producer or RV64 materialization edit.
- Narrow immediate offsets remain on the existing local-memory consumer route.
  This idea owns only the large-offset materialization boundary.
- Missing selected base, offset, width, address-space, freshness, range,
  scratch, or clobber facts should fail closed with a precise owner diagnosis.
- Reclassify rows into an existing or new source idea when fresh evidence
  proves they are producer authority gaps or unrelated local-memory owners.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Do not edit the source idea unless durable intent changes or closure notes
  are required.
- Add focused prepared-layer or RV64 object-emission tests for code-changing
  steps.
- Use `cmake --build --preset default` plus a supervisor-selected focused
  backend subset as the normal proof ladder for code slices.
- Treat expectation rewrites, unsupported-marker changes, testcase-shaped
  source matching, filename matching, exact-offset special casing,
  final-assembly inference, and diagnostic-only changes as route failures.

## Step 1: Refresh Large Selected Pointer-Offset Evidence

Goal: identify the current large selected pointer-offset local-memory rows and
their first owner diagnostics.

Actions:
- Re-run focused probes for large selected pointer-offset local-memory rows
  represented by `src/ipa-sra-2.c` and `src/pr60822.c`.
- Capture selected base identity, selected offset, access width, address
  space, memory-use freshness, offset range classification, current rejection
  point, and any visible scratch or clobber facts.
- Separate large selected pointer offsets from narrow frame-slot accesses,
  producer-owned pointer rows, direct pointer arithmetic, string/global rows,
  aggregate homes, unsupported widths, and unrelated runtime/accounting
  failures.
- Record row evidence and the recommended next owner bucket in `todo.md`.

Completion check:
- `todo.md` lists the rows inspected, the large-offset facts, the first
  missing or rejecting boundary, and a next step that is not named-case-only.

## Step 2: Trace Selected Pointer And Range Authority

Goal: locate the prepared carrier boundary that should expose selected
base-plus-offset facts to RV64 materialization.

Actions:
- Trace selected local-memory access construction for the Step 1 in-scope
  bucket.
- Identify the carrier fields that already hold, or should hold, selected
  base identity, offset, access width, address space, memory-use freshness,
  and offset range facts.
- Locate the first missing, stale, ambiguous, or unsupported authority
  boundary before RV64 object emission.
- Record fail-closed states for missing selected base, absent offset, stale
  memory-use freshness, incomplete width or address-space facts, ambiguous
  pointer base, unsupported range, and mismatched selected memory-use
  authority.

Completion check:
- `todo.md` names the exact producer functions, carrier fields, consumer
  checks, and the smallest code-changing packet that can publish, verify, or
  consume large selected pointer-offset authority.

## Step 3: Define Scratch And Clobber Materialization Contract

Goal: make the RV64 large-offset materialization preconditions explicit before
emission uses a scratch register.

Actions:
- Inspect the RV64 object-emission path for available scratch-register and
  clobber-safety facts at the selected local-memory access point.
- Define the minimum admission contract for materializing a large offset into
  an address register before load/store emission.
- Preserve fail-closed behavior when scratch availability, clobber safety, or
  supported offset range is absent or ambiguous.
- Keep narrow immediate frame-slot accesses on the existing path.

Completion check:
- `todo.md` records the scratch/clobber facts available to the RV64 consumer,
  the missing authority if any, and whether the next packet is producer
  publication, consumer admission, or a lifecycle split.

## Step 4: Add Narrow RV64 Large-Offset Consumer Admission

Goal: allow RV64 object emission to materialize only explicit large selected
pointer-offset local-memory accesses for the selected family.

Actions:
- Replace the relevant large-offset `unsupported_local_memory_access`
  rejection with validation of selected pointer, range, freshness, scratch,
  and clobber authority.
- Emit a target-valid materialization sequence only when selected base,
  selected offset, width, address space, freshness, range, scratch, and
  clobber facts match the consumer contract.
- Add focused positive coverage plus fail-closed tests for absent, malformed,
  stale, ambiguous, mismatched, unsupported, scratchless, and clobber-unsafe
  facts.
- Preserve existing frame-slot behavior and keep producer-owned pointer rows,
  direct pointer arithmetic, string/global rows, aggregate homes, unsupported
  widths, and unrelated owners out of this admission path.

Completion check:
- Focused positive and negative backend tests pass, and the RV64 path rejects
  large selected pointer-offset local-memory accesses without explicit
  selected authority and scratch/clobber safety.

## Step 5: Reclassify Large Selected Pointer-Offset Rows

Goal: determine whether the source idea is complete, needs another
large-offset policy packet, or should split remaining work into separate
initiatives.

Actions:
- Re-run the large selected pointer-offset probes after any Step 2, Step 3, or
  Step 4 changes.
- Classify each remaining failure into selected pointer authority,
  freshness/range authority, scratch/clobber target policy, RV64 consumer
  admission, or an out-of-scope owner bucket.
- Record whether idea `634` is close-ready or which next large-offset
  materialization packet is justified.

Completion check:
- `todo.md` contains row-by-row classification, proof results, and a clear
  close/split/continue recommendation for the supervisor.
