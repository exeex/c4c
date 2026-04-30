# LIR/BIR Backend Aggregate Layout Text Bridge Cleanup Runbook

Status: Active
Source Idea: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md

## Purpose

Retire semantic aggregate layout authority that still crosses the LIR to
BIR/backend boundary through final `%type` spelling or legacy textual
type-declaration bodies when structured aggregate layout carriers are
available.

Goal: make structured aggregate layout data the primary semantic authority
without deleting compatibility paths that raw or hand-built LIR still needs.

## Core Rule

Prefer `LirStructDecl`, `StructNameId`, structured type refs, or an equivalent
structured aggregate layout table whenever producer data exists. Final `%type`
text may remain only as emitted artifact spelling, diagnostics, dumps, or an
explicit compatibility fallback with visible mismatch behavior.

## Read First

- `ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md`
- LIR type declaration and structured struct declaration definitions
- `LirModule::type_decls`
- `TypeDeclMap`
- `compute_aggregate_type_layout()`
- `lookup_backend_aggregate_type_layout()`
- `BackendStructuredLayoutTable`
- `StructuredTypeSpellingContext`
- Aggregate initializer and global initializer layout consumers

## Current Targets

- LIR type declarations and structured struct declarations
- BIR aggregate layout import
- Backend structured layout tables
- Aggregate initializer layout handling
- Global initializer layout handling
- Focused same-feature tests for structured-present, structured-missing,
  mismatch, aggregate initializer, and global initializer paths

## Non-Goals

- Do not remove final LLVM or assembly type spellings that are emitted
  artifacts.
- Do not bulk-delete legacy `type_decls` before raw or hand-built LIR has an
  explicit fallback contract.
- Do not change Parser, Sema, HIR owner/member/template lookup cleanup.
- Do not rework unrelated link-name, block-label, value-name, local-slot,
  diagnostic, dump, inline asm, register-name, or final assembly text paths.
- Do not add testcase-shaped matching or named-case shortcuts.

## Working Model

Structured layout carriers should form the primary route:

1. LIR producers emit structured aggregate declarations where they can.
2. BIR/backend import preserves structured layout identity and member layout
   without rederiving it from final type text.
3. Legacy textual type-declaration parsing remains a compatibility fallback
   only when structured data is absent.
4. Structured/textual disagreement is reported or rejected through an explicit
   mismatch path instead of silently letting legacy text override structured
   data.

## Execution Rules

- Keep each code change small enough to prove with a fresh build or narrow
  focused test subset.
- Preserve behavior for raw or hand-built LIR that lacks structured aggregate
  layout data.
- Make fallback and mismatch behavior observable in tests or diagnostics.
- When a step touches shared backend import or initializer behavior, include a
  broader checkpoint after the narrow proof.
- Update `todo.md` for executor packet progress; rewrite this runbook only
  when the route contract changes.

## Steps

### Step 1: Trace Current Aggregate Layout Authority

Goal: identify every path where aggregate layout decisions still depend on
final `%type` text or legacy textual type-declaration bodies.

Primary target: LIR declarations, BIR import, backend layout lookup, aggregate
initializer layout, and global initializer layout consumers.

Actions:

- Inspect `LirModule::type_decls`, structured struct declaration storage, and
  `StructNameId` use.
- Trace `TypeDeclMap`, `compute_aggregate_type_layout()`,
  `lookup_backend_aggregate_type_layout()`, `BackendStructuredLayoutTable`, and
  `StructuredTypeSpellingContext`.
- Record which paths already prefer structured data and which paths still parse
  or trust final type text.
- Identify the smallest first code packet that can move one consumer to
  structured-primary lookup without changing unrelated emitted text.

Completion check:

- `todo.md` names the selected first implementation packet and the proof
  command the supervisor should delegate.
- The trace distinguishes structured-primary, fallback-only, and suspicious
  text-authority routes.

### Step 2: Make Backend Layout Lookup Structured-Primary

Goal: ensure backend aggregate layout lookup uses structured layout carriers
when available and treats textual layout parsing as compatibility fallback.

Primary target: `lookup_backend_aggregate_type_layout()`,
`BackendStructuredLayoutTable`, and related import helpers.

Actions:

- Route lookup through structured aggregate identity before final `%type`
  spelling.
- Keep legacy textual lookup only for structured-missing compatibility cases.
- Preserve or add parity checks between structured and textual layout data.
- Make structured/text mismatch visible instead of silently accepting the
  textual result.

Completion check:

- Structured-present cases no longer require final type spelling as semantic
  layout authority.
- Structured-missing legacy cases still work through an explicit fallback.
- Narrow backend/LIR tests and a fresh build pass.

### Step 3: Move Aggregate Initializer Layout Consumers Off Text Authority

Goal: make aggregate initializer layout decisions consume structured layout
data when present.

Primary target: aggregate initializer lowering and layout parsing routes that
currently rederive layout from final type text or textual type-decl bodies.

Actions:

- Thread structured layout identity or lookup results into aggregate
  initializer consumers.
- Keep textual parsing isolated to the structured-missing fallback.
- Add coverage where structured data is present and final spelling alone would
  be insufficient or misleading.

Completion check:

- Aggregate initializer tests prove structured-present behavior and
  structured-missing fallback behavior.
- The diff does not introduce named-testcase shortcuts or expectation
  downgrades.

### Step 4: Move Global Initializer Layout Consumers Off Text Authority

Goal: make global initializer layout decisions consume structured layout data
when present.

Primary target: global initializer lowering and backend import/layout lookup
routes.

Actions:

- Reuse the structured-primary lookup contract from earlier steps.
- Quarantine textual type-decl body parsing to explicit fallback logic.
- Add focused global initializer coverage for structured-present,
  structured-missing, and mismatch paths where practical.

Completion check:

- Global initializer tests cover same-feature behavior beyond one narrow named
  case.
- Narrow tests and a fresh build pass.

### Step 5: Consolidate Fallback and Mismatch Reporting

Goal: make compatibility fallback and structured/text mismatch handling clear,
centralized, and testable.

Primary target: helper APIs around aggregate layout lookup and text parsing.

Actions:

- Extract or tighten helper names so callers can distinguish
  structured-primary, fallback-used, and mismatch cases.
- Remove duplicated ad hoc textual layout authority from callers that can use
  the central contract.
- Ensure diagnostics, dumps, or final emitted text remain display/emission
  concerns, not semantic layout authorities.

Completion check:

- Callers no longer need to parse final `%type` text when structured layout
  data is present.
- Mismatch/fallback behavior is visible in focused tests.
- A broader backend/LIR validation checkpoint passes.

### Step 6: Final Coverage and Regression Check

Goal: prove the idea acceptance criteria without relying on one named testcase.

Actions:

- Run focused tests for structured-present, structured-missing fallback,
  mismatch, aggregate initializer, and global initializer paths.
- Run a broader validation command selected by the supervisor, such as
  `ctest --test-dir build -j --output-on-failure` or the matching
  `c4c-regression-guard` scope.
- Inspect the diff for expectation downgrades or testcase-overfit patterns.

Completion check:

- Acceptance criteria from the source idea are satisfied.
- `todo.md` records proof commands and results.
- The supervisor has enough evidence to decide whether the source idea can be
  closed or needs another runbook.
