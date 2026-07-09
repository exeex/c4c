# Direct Global-Symbol Local-Memory Policy Runbook

Status: Active
Source Idea: ideas/open/631_direct_global_symbol_local_memory_policy.md

## Purpose

Define the RV64 consumer policy for local-memory accesses whose selected base
is a direct `global_symbol`, keeping that route separate from prepared global
value-location handling.

## Goal

Move at least one direct global-symbol local-memory row past its current owner,
or reclassify the family to a precise prepared/global producer owner with
current diagnostics.

## Core Rule

RV64 may consume a direct global-symbol local-memory access only when explicit
facts identify the global symbol, base-plus-offset, width, extent, addressing
mode, and selected memory-use authority. Do not infer those facts from source
filenames, final symbol spelling, final assembly layout, or prepared
value-location carriers owned by idea `621`.

## Read First

- `ideas/open/631_direct_global_symbol_local_memory_policy.md`
- `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
- `ideas/open/621_rv64_prepared_global_value_location_consumer.md` if present
  in the checkout or current history.
- `ideas/closed/608_prepared_global_data_authority.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- RV64 local-memory consumers in
  `src/backend/mir/riscv/codegen/object_emission.cpp`, especially selected
  base classification, base-plus-offset emission, and
  `unsupported_local_memory_access` diagnostics.

## Current Targets

- Local-memory rows whose selected base is a direct `global_symbol`.
- Prepared facts for global identity, selected offset, access width, object
  extent, address space or relocation/addressing mode, and selected
  local-memory use authority.
- Diagnostic rows that are not prepared global value-location sequences, string
  constants, aggregate stack homes, generic frame slots, unsupported
  F128/16-byte accesses, or large selected pointer-offset materializations.

## Non-Goals

- Prepared global value-location consumption owned by idea `621`.
- Prepared/global producer authority owned by idea `608`, unless refreshed
  evidence proves this route cannot proceed without a separate producer split.
- String-constant local-memory policy from idea `630`.
- Aggregate, sret, byval, or pointer stack-home local-memory policy from idea
  `633`.
- Large selected pointer-offset materialization policy from idea `634`.
- Generic frame-slot local-memory support already closed by idea `614`.
- ABI, runtime/library policy, expectation changes, unsupported marker changes,
  allowlists, timeouts, or accounting.

## Working Model

- Evidence comes first. Refresh the direct global-symbol rows before choosing a
  producer or consumer edit.
- Direct `global_symbol` bases are not prepared global value-location
  sequences. Reclassify rows into idea `621` only when fresh diagnostics prove
  they use that prepared route without broadening idea `621`.
- Consumer admission should be narrow and fact-driven. It may unblock a row
  only when explicit direct global-symbol authority exists.
- Reclassify rows that prove to be producer gaps, string-constant rows,
  aggregate-home rows, large-offset rows, unsupported-width rows, or unrelated
  local-memory owners instead of broadening this idea.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Do not edit the source idea unless durable intent changes or closure notes
  are required.
- Add focused prepared-layer or RV64 object-emission tests for any
  code-changing step.
- Use `cmake --build --preset default` plus a supervisor-selected backend
  subset as the normal proof ladder for code slices.
- Treat expectation rewrites, unsupported-marker changes, testcase-shaped
  source matching, filename matching, final-assembly inference, and symbol-name
  special casing as route failures.

## Step 1: Refresh Direct Global-Symbol Evidence

Goal: identify the current direct global-symbol local-memory rows and their
first owner diagnostics.

Actions:
- Re-run focused probes for rows currently reported as direct `global_symbol`
  local-memory residuals from the idea-614 classification.
- Capture selected base kind, global identity, selected offset, access width,
  extent or object-size facts, address space or relocation/addressing mode,
  memory-use authority, and current failure bucket labels.
- Separate direct global-symbol rows from prepared global value-location rows,
  string constants, aggregate homes, large selected pointer offsets, generic
  frame slots, unsupported widths, and producer-owned rows.
- Record row evidence and the recommended next owner bucket in `todo.md`.

Completion check:
- `todo.md` lists the rows inspected, the current direct global-symbol facts,
  the RV64 rejection or emission point, and a next step that is not
  named-case-only.

## Step 2: Trace Direct Global-Symbol Authority Carriers

Goal: locate the producer and carrier boundary that should expose direct
global-symbol base-plus-offset authority to RV64.

Actions:
- Trace selected local-memory access construction for the Step 1 in-scope
  bucket.
- Identify the carrier fields that already hold, or should hold, global
  identity, symbol reference, offset, access width, extent, address space,
  relocation/addressing mode, and memory-use authority.
- Locate the first missing or ambiguous authority boundary before RV64 object
  emission.
- Record fail-closed states for missing global identity, unsupported
  addressing, incomplete extent or width facts, ambiguous object-data facts,
  and mismatched selected memory-use authority.

Completion check:
- `todo.md` names the exact producer functions, carrier fields, consumer
  checks, and the smallest code-changing packet that can publish or consume
  direct global-symbol local-memory authority.

## Step 3: Publish Or Verify Prepared Direct-Global Facts

Goal: ensure direct global-symbol local-memory facts are explicit before RV64
lowering depends on them.

Actions:
- If upstream direct-global authority exists but is not published on the
  selected local-memory carrier, add the narrow producer publication path.
- If publication already exists, add focused coverage proving the facts are
  complete before object emission.
- Preserve global identity, offset, width, extent, address space,
  relocation/addressing mode, and selected memory-use authority.
- Keep prepared global value-location rows out of this direct-global carrier
  unless a separate lifecycle decision moves them into idea `621`.

Completion check:
- Focused backend coverage proves the selected direct global-symbol facts are
  present in the prepared representation, or `todo.md` reclassifies the bucket
  with precise missing upstream authority.

## Step 4: Add Narrow RV64 Direct-Global Consumer Admission

Goal: allow RV64 object emission to consume only explicit direct global-symbol
local-memory facts for the selected family.

Actions:
- Replace the broad `unsupported_local_memory_access` rejection for the
  selected family with validation of direct global-symbol local-memory
  authority.
- Emit only when global identity, selected offset, access width, extent,
  address space or relocation/addressing mode, and selected memory-use
  authority match the consumer contract.
- Add fail-closed tests for absent, malformed, ambiguous, mismatched, or
  unsupported direct-global facts.
- Preserve existing frame-slot behavior and keep prepared value-location,
  string-constant, aggregate-home, large-offset, unsupported-width, and
  unrelated owner families out of this admission path.

Completion check:
- Focused positive and negative backend tests pass, and the RV64 path rejects
  direct global-symbol local-memory accesses without explicit prepared
  authority.

## Step 5: Reclassify Direct-Global Rows

Goal: determine whether the source idea is complete, needs another
direct-global policy packet, or should split remaining work into separate
initiatives.

Actions:
- Re-run the direct global-symbol row probes after any Step 3 or Step 4
  changes.
- Classify each remaining failure into direct global-symbol local-memory
  authority or an out-of-scope owner bucket.
- Record whether idea 631 is close-ready or which next direct-global
  local-memory packet is justified.

Completion check:
- `todo.md` contains row-by-row classification, proof results, and a clear
  close/split/continue recommendation for the supervisor.
