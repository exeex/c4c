# Parser Rendered Record Template Lookup Mirror Cleanup Reopen Repair Runbook

Status: Active
Source Idea: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md

## Purpose

Repair the reopened idea 132 close regression exposed by a rebuilt
`frontend_lir_function_signature_type_ref` test.

## Goal

Find and fix the structured identity propagation path that lets
`declared_pair` receive a `LirFunction.signature_return_type_ref` mirror whose
structured return type name disagrees with the printed `%struct.Big` return
type.

## Core Rule

Do not overfit the failing rebuilt test. The repair must address the general
record/template structured identity or LIR signature metadata authority path,
not special-case `declared_pair`, weaken verifier checks, downgrade test
expectations, or hide mismatch diagnostics.

## Read First

- `ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md`
- `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- `src/codegen/lir/verify.cpp`
- parser/HIR record and template identity handoff code touched by idea 132

## Current Targets

- Reproduce and inspect the rebuilt failure for
  `frontend_lir_function_signature_type_ref`.
- Trace the `declared_pair` return type from parser record/template identity
  production through HIR and LIR signature metadata.
- Identify whether the mismatch comes from parser structured identity
  propagation, HIR-to-LIR type-ref construction, stale compatibility mirror
  lookup, or verification expectations.
- Repair the authority path with nearby same-feature coverage where needed.

## Non-Goals

- Do not edit implementation from the plan-owner role.
- Do not redesign parser template instantiation.
- Do not remove generated artifact spelling or diagnostic text.
- Do not weaken `frontend_lir_function_signature_type_ref` or the LIR verifier.
- Do not accept the pending baseline review while this rebuilt test fails.

## Working Model

- Idea 132 remains about keeping rendered record/template spelling
  compatibility-only once structured identity is available.
- The failing rebuilt LIR test is a downstream proof that a structured mirror can
  disagree with the printed function signature after the close commit.
- The repair should follow the structured authority chain backward from the LIR
  mismatch until the first incorrect identity handoff is found.

## Execution Rules

- Keep routine implementation progress and proof details in `todo.md`.
- Prefer semantic identity propagation or mirror-authority fixes over
  testcase-shaped matching.
- Preserve mismatch visibility; do not silence verifier failures unless the
  verifier is proven wrong and coverage proves the corrected contract.
- Each code-changing executor packet needs fresh build or narrow test proof.
- Supervisor should require rebuilt proof before any renewed close attempt.

## Ordered Steps

### Step 1: Reproduce And Localize Signature Type-Ref Mismatch

Goal: Establish the first bad fact for the rebuilt
`frontend_lir_function_signature_type_ref` failure.

Primary target: `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp`
and the LIR/HIR dump or debug path needed to compare printed signature text,
`signature_return_type_ref`, and structured `StructNameId`.

Actions:

- Run the delegated narrow proof command from a clean rebuilt state.
- Inspect the generated module for `declared_pair`, `%struct.Big`, and the
  structured return type-ref mirror.
- Trace whether the wrong identity appears before HIR-to-LIR lowering, during
  LIR signature metadata construction, or during verification.
- Record the localized authority boundary in `todo.md`.

Completion check:

- `todo.md` names the first code location where the structured return type
  diverges from the expected record identity, with the failing proof command and
  output summarized.

### Step 2: Repair The Structured Authority Path

Goal: Fix the general identity propagation or mirror construction bug that
causes the mismatched return type-ref.

Primary target: the first incorrect handoff found in Step 1.

Actions:

- Replace any rendered-spelling-derived semantic choice with the available
  structured record/template identity.
- If the bug is in LIR signature metadata construction, ensure
  `signature_return_type_ref` receives the same structured record identity as
  the function signature type.
- Preserve compatibility fallback only where structured identity is genuinely
  unavailable and make fallback/mismatch behavior visible.
- Add or adjust nearby same-feature coverage only when it proves the general
  path rather than the single failing name.

Completion check:

- The narrow rebuilt `frontend_lir_function_signature_type_ref` proof passes,
  and `todo.md` documents why the repair is semantic rather than
  testcase-specific.

### Step 3: Baseline Review And Reclose Readiness

Goal: Re-establish close readiness for idea 132 with rebuilt proof.

Actions:

- Re-run the supervisor-selected baseline or regression guard after the narrow
  fix passes.
- Confirm no pending baseline review failure remains.
- Summarize final proof and any residual risk in `todo.md`.

Completion check:

- Supervisor has matching fresh proof sufficient to request plan-owner close
  review again.
