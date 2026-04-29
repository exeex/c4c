# LIR Structured Function Signature Metadata Boundary Runbook

Status: Active
Source Idea: ideas/open/127_lir_structured_function_signature_metadata_boundary.md

## Purpose

Stop using `LirFunction::signature_text` as a semantic authority for function
signature facts across the LIR -> BIR/backend boundary.

## Goal

Thread structured LIR function signature metadata through backend/BIR lowering
and LIR verification so rendered LLVM header text remains final spelling,
diagnostic, dump, or render-consistency data only.

## Core Rule

Do not make backend or verifier decisions by parsing final rendered function
header spelling when the same fact can be carried by structured LIR metadata.

## Read First

- `ideas/open/127_lir_structured_function_signature_metadata_boundary.md`
- LIR function model and printer definitions for `LirFunction`
- HIR-to-LIR production of `return_type`, `params`,
  `signature_return_type_ref`, and `signature_param_type_refs`
- BIR/backend function lowering paths that currently consume
  `signature_text`
- LIR verifier signature checks

## Current Targets

- Function return and parameter signature facts
- Variadic declaration/definition metadata
- Aggregate, byval, sret, and ABI-relevant signature classification data
- LIR verifier checks for function signature metadata
- Tests that prove structured metadata is authoritative even when
  `signature_text` drifts

## Non-Goals

- Do not remove `LirFunction::signature_text`.
- Do not change final LLVM header rendering except where needed to preserve
  existing behavior.
- Do not redesign the broad type system.
- Do not alter parser, Sema, or HIR lookup policy beyond signature metadata
  already needed by HIR-to-LIR.
- Do not weaken supported tests or replace semantic checks with string-shaped
  shortcuts.

## Working Model

- `signature_text` is the printer-facing final LLVM function header spelling.
- Structured LIR fields are the semantic source for cross-IR return,
  parameter, variadic, aggregate, and ABI facts.
- Backend/BIR lowering may retain `signature_text` only for diagnostics,
  dumps, render text, or explicitly documented fallback paths.
- LIR verifier may compare rendered text for final-spelling consistency, but
  semantic signature validation should start from structured metadata.

## Execution Rules

- Keep each step behavior-preserving unless the step explicitly changes the
  authority source from rendered text to structured metadata.
- Prefer adding narrow structured carriers or access helpers over broad model
  rewrites.
- When a `signature_text` consumer remains, classify it as render,
  diagnostic, dump, consistency-check, fallback, or suspicious semantic
  authority.
- If a suspicious semantic consumer cannot be fixed inside this runbook, record
  it as a follow-up idea under `ideas/open/` instead of leaving it implicit.
- For code-changing steps, prove with build plus the narrow tests selected by
  the supervisor or executor packet; escalate to broader validation before
  treating the idea as complete.

## Steps

### Step 1: Inventory Signature Text Consumers

Goal: separate legitimate final-text uses from semantic authority uses.

Primary targets:
- LIR model, printer, and verifier code
- BIR/backend function-signature lowering code
- Any helper that parses, scans, or branches on `LirFunction::signature_text`

Actions:
- Search for `signature_text` reads and signature-header parsing helpers.
- Classify each use as printer/render, diagnostic/dump, consistency check,
  fallback, or semantic lowering authority.
- Identify the structured metadata each semantic use should consume instead.
- Record any remaining suspicious consumer that is outside this runbook's
  bounded signature-metadata scope.

Completion check:
- The executor can name every semantic `signature_text` consumer that must be
  converted before this idea can close.
- No implementation behavior has changed unless needed for mechanical
  instrumentation or compile fixes.

### Step 2: Complete Structured LIR Signature Metadata

Goal: ensure LIR carries the return, parameter, variadic, aggregate, and
ABI-relevant facts currently recovered from rendered header text.

Primary targets:
- HIR-to-LIR function emission
- `LirFunction` structured signature fields
- Type-reference metadata used by BIR/backend lowering

Actions:
- Inspect existing `return_type`, `params`, `signature_return_type_ref`, and
  `signature_param_type_refs` production.
- Add or thread only the missing structured metadata needed by downstream
  lowering and verifier checks.
- Keep final `signature_text` generation behavior stable.
- Add local assertions or verifier checks only when they validate structured
  metadata consistency rather than shifting authority back to rendered text.

Completion check:
- Structured LIR function data can answer the backend/verifier signature facts
  found in Step 1 without parsing `signature_text`.
- Build proof is fresh after metadata shape changes.

### Step 3: Convert Backend/BIR Signature Lowering

Goal: make backend/BIR function lowering consume structured signature metadata
as the primary authority.

Primary targets:
- BIR function construction and backend lowering helpers that recover return or
  parameter facts from final header spelling
- ABI decision points for aggregate returns, byval/sret, parameter lowering,
  variadic calls, declarations, and definitions

Actions:
- Replace semantic parsing of `signature_text` with structured LIR metadata.
- Preserve diagnostic or dump access to `signature_text` where appropriate.
- Remove or downgrade parsing helpers once no semantic caller remains.
- Avoid testcase-shaped branches; the conversion must apply to the signature
  fact category, not to one named fixture.

Completion check:
- Backend/BIR lowering no longer parses `signature_text` for covered
  return/parameter/ABI facts.
- Narrow backend/BIR tests selected by the supervisor or executor pass.

### Step 4: Convert LIR Verifier Signature Checks

Goal: make the LIR verifier validate semantic signature facts from structured
metadata first.

Primary targets:
- LIR verifier function-signature checks
- Existing signature mirror consistency checks

Actions:
- Change semantic checks to read structured return, parameter, variadic, and
  type-reference metadata.
- Retain rendered-header parsing only for final-render consistency checks or
  fallback diagnostics, with comments that make that role explicit.
- Ensure verifier diagnostics still explain mismatches clearly.

Completion check:
- Verifier semantic signature checks no longer depend on parsing final header
  spelling.
- Any retained `signature_text` parsing is clearly classified as render
  consistency or fallback diagnostics.

### Step 5: Add Drift-Resistance Tests

Goal: prove structured metadata, not rendered `signature_text`, is the
cross-IR semantic authority.

Primary targets:
- Focused unit or integration tests around LIR -> BIR/backend lowering and LIR
  verifier behavior

Actions:
- Add tests that corrupt or drift rendered signature spelling while keeping
  structured metadata correct where the test harness can do so directly.
- Cover representative return, parameter, variadic, and aggregate/ABI facts
  based on the consumers found in Step 1.
- Keep final-render tests separate from semantic lowering tests so authority
  roles stay visible.

Completion check:
- Tests fail if backend/BIR or verifier semantic checks return to
  `signature_text` parsing for the covered facts.
- The proof command writes the canonical executor result to `test_after.log`
  when delegated by the supervisor.

### Step 6: Final Audit And Follow-Up Queue

Goal: verify the cleanup boundary and record any remaining suspicious string
authority explicitly.

Actions:
- Re-run the `signature_text` and signature-parser inventory from Step 1.
- Confirm remaining uses are render, diagnostic, dump, consistency-check, or
  documented fallback paths.
- Create a focused follow-up idea under `ideas/open/` for any remaining
  semantic authority that is real but outside this plan.
- Run the supervisor-selected broader validation checkpoint before closure.

Completion check:
- Acceptance criteria from the source idea are satisfied or every remaining
  gap is captured in a concrete follow-up idea.
- The source idea can be judged for closure separately from runbook exhaustion.
