# LIR Structured Signature Reference Producer Boundary Runbook

Status: Active
Source Idea: ideas/open/127_lir_structured_signature_reference_producer_boundary.md

## Purpose

Move LIR function-signature reference reachability away from rendered
`signature_text` scanning and onto a structured producer-carried reference
surface.

## Goal

Function references embedded in LIR signatures should be produced as structured
metadata before reachability runs, with `signature_text` retained only as final
LLVM header spelling, display, diagnostics, dumps, or compatibility payload.

## Core Rule

Rendered `signature_text` is not primary semantic authority for covered
signature references. Prefer producer-carried structured reference identity and
keep any retained text scanning explicitly classified as fallback or
compatibility behavior.

## Read First

- `ideas/open/127_lir_structured_signature_reference_producer_boundary.md`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/verify.cpp`
- `src/codegen/lir/lir_printer.cpp`
- `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp`
- `tests/frontend/frontend_hir_tests.cpp`

## Current Targets

- `collect_fn_refs` and reachability logic in
  `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`.
- The HIR-to-LIR producer path that builds `signature_text` and populates
  signature type reference mirrors.
- `LirFunction` fields that can carry signature reference metadata.
- Focused frontend LIR tests proving structured signature references survive
  drifted rendered spelling.

## Non-Goals

- Broad type-reference redesign outside signature reference reachability.
- Parser source lookup cleanup.
- MIR or target assembly behavior changes.
- Removing final emitted LLVM header spelling.
- Downgrading supported tests, marking covered paths unsupported, or adding
  named-case shortcuts.

## Working Model

- Step 1 inventories the exact signature reference families currently recovered
  by scanning `signature_text`.
- The new carrier should be populated by the same producer boundary that knows
  the semantic signature references, not rediscovered from rendered text.
- LIR reachability should consume structured references first and retain
  `signature_text` scanning only for uncovered compatibility cases.
- Verification and tests should make drift behavior explicit so rendered
  spelling changes cannot silently regain semantic authority.

## Execution Rules

- Keep packets narrow and behavior-preserving except where tests currently
  encode legacy `signature_text` authority.
- Do not weaken existing verifier coverage for signature type-ref mirrors.
- Do not convert final spelling, printer output, diagnostics, or dump text into
  semantic lookup authority.
- Any missing upstream producer data that cannot be solved in this LIR boundary
  must become a separate open idea instead of being hidden in this runbook.
- Every code-changing packet needs fresh build or compile proof plus the
  supervisor-selected focused frontend LIR test subset.
- Escalate to broader frontend or backend-route validation when changes affect
  shared LIR IR shape, verifier contracts, or HIR-to-LIR reachability.

## Step 1: Inventory Signature Reference Producer And Consumer Surfaces

Goal: identify the exact signature references still recovered through rendered
`signature_text` scanning and where producer-owned metadata can carry them.

Primary targets: `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`,
`src/codegen/lir/ir.hpp`, and focused LIR signature tests.

Actions:
- Inspect `collect_fn_refs`, `scan_refs`, signature construction, signature
  type-ref mirror population, and function reachability pruning.
- Classify each current `signature_text` use as reachability authority, final
  spelling, verifier mirror, diagnostics, dump text, printer input, or
  compatibility fallback.
- Identify the first bounded carrier shape for function references embedded in
  signatures without redesigning unrelated type references.
- Record packet notes in `todo.md`; do not edit the source idea for routine
  inventory findings.

Completion check:
- `todo.md` names the first implementation packet and records which
  `signature_text` scans remain semantic authority versus classified fallback
  or display behavior.

## Step 2: Add The Structured Signature Reference Carrier

Goal: add the minimal LIR data surface needed to represent signature function
references produced before reachability analysis.

Primary targets: `src/codegen/lir/ir.hpp` and the HIR-to-LIR producer helpers.

Actions:
- Add or thread a structured `LirFunction` carrier for signature function
  references, using existing link-name or function identity types when
  available.
- Populate the carrier in the producer path that already emits function
  signatures and signature type-ref mirrors.
- Keep `signature_text` intact for final LLVM header spelling and existing
  printer behavior.
- Add verifier checks only where they protect carrier consistency without
  reintroducing string parsing as primary authority.

Completion check:
- The carrier is populated for covered signature references before reachability
  runs.
- Existing focused frontend LIR signature tests still pass after build proof.

## Step 3: Route Reachability Through Structured Signature References

Goal: make LIR reachability consume producer-carried signature references before
falling back to rendered text scanning.

Primary target: `collect_fn_refs` and its callers in
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`.

Actions:
- Teach reference collection to merge structured signature references into the
  function reference set.
- Demote `signature_text` scanning to explicit fallback for uncovered
  compatibility cases, with comments or naming that make that status visible.
- Preserve existing structured reference paths for calls, globals, and
  initializer references.
- Avoid testcase-shaped matching for one known signature spelling.

Completion check:
- Covered signature references no longer depend on rendered `signature_text`
  spelling for reachability.
- A focused test proves a function remains reachable through structured
  signature references when rendered signature spelling drifts.

## Step 4: Strengthen Drift And Verifier Coverage

Goal: prove structured signature reference identity wins and retained text use
is still correctly bounded.

Primary targets: `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp`
and nearby frontend HIR-to-LIR reachability tests.

Actions:
- Add or update tests that corrupt rendered signature spelling while preserving
  structured signature references.
- Cover both declaration and definition paths when both participate in the
  producer boundary.
- Keep tests semantic: assert reachability or structured identity, not just
  exact incidental text.
- Preserve verifier tests that reject mismatched signature type-ref mirrors.

Completion check:
- Focused frontend LIR tests fail without the structured carrier path and pass
  with it.
- No supported test is weakened or converted to an unsupported expectation.

## Step 5: Classify Remaining Signature Text Uses

Goal: leave every retained `signature_text` path visibly non-authoritative or
explicitly unresolved.

Primary targets: remaining `signature_text` consumers in LIR, verifier,
printer, and backend route boundaries touched or inspected by this work.

Actions:
- Rename helpers, parameters, comments, or tests where needed to classify
  retained rendered signature text as final spelling, display, diagnostics,
  dump text, verifier compatibility mirror, or unresolved boundary.
- Keep compatibility parsing where needed, but do not present it as primary
  semantic reachability authority for covered signature references.
- If a separate backend signature parser migration is discovered, record it as
  a separate open idea rather than expanding this plan.

Completion check:
- Covered LIR signature reference reachability no longer treats
  `signature_text` as primary authority.
- Remaining signature text consumers have a clear purpose in code, tests, or
  `todo.md`.

## Step 6: Reprove And Decide Closure Or Split

Goal: validate the LIR producer-boundary cleanup and decide whether the source
idea is complete or needs a follow-up split.

Actions:
- Run the supervisor-selected build and focused frontend LIR proof, escalating
  to broader frontend/backend-route validation if shared LIR IR shape or
  reachability behavior changed.
- Review the diff for testcase-overfit, unsupported downgrades, and retained
  `signature_text` semantic authority.
- If the producer-boundary scope is complete, prepare for lifecycle close.
- If unresolved backend or upstream producer work remains, write or request a
  separate open idea and keep this source idea focused.

Completion check:
- Regression proof is green for the selected close scope.
- The plan owner can decide whether to close this source idea or split a
  follow-up without re-inventorying all signature text consumers.
