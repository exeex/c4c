# Frontend-to-BIR Legacy String Lookup Closure Gate Runbook

Status: Active
Source Idea: ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md

## Purpose

Decide whether the second frontend-to-BIR legacy string lookup audit is closed
and whether backend restart is allowed next.

## Goal

Validate that parser, sema, HIR, LIR, and BIR retained strings are either
structured-authority mirrors, display/output text, diagnostics, route-local
handles, ABI/final spelling, or explicit no-metadata compatibility.

## Core Rule

Do not start backend restart work in this runbook. This gate only classifies
the remaining frontend-to-BIR surfaces, validates the milestone, and records a
closure decision or a new blocker idea.

## Read First

- `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`
- `ideas/closed/188_lir_bir_freeze_closure_gate.md`
- `ideas/closed/190_lir_call_argument_structured_payload_boundary.md`
- `ideas/closed/191_bir_function_signature_byval_metadata_text_retirement.md`
- `ideas/closed/192_hir_compile_time_rendered_registry_api_retirement_audit.md`
- `ideas/closed/193_hir_constructor_member_owner_structured_lookup_closure.md`
- `ideas/closed/194_bir_global_memory_provenance_linknameid_expansion.md`
- `ideas/closed/201_hir_template_registry_structured_generated_paths.md`
- `ideas/closed/202_hir_generated_member_payload_structured_miss.md`

## Scope

- Reconstruct the frontend-to-BIR closure ledger from the completed dependency
  ideas and the two resolved HIR blockers.
- Recheck high-risk semantic lookup surfaces for complete structured miss
  recovery through rendered strings.
- Run milestone-appropriate validation before closure.
- Decide whether idea 195 can close, must create a new narrow blocker, or must
  stay open for a specific documented reason.

## Non-Goals

- Do not implement backend restart.
- Do not remove diagnostic, printer, assembler, ABI, final spelling,
  route-local SSA, slot, or block handle strings.
- Do not rewrite parser, sema, HIR, LIR, or BIR type systems.
- Do not downgrade expectations or mark supported paths unsupported to claim
  closure.

## Working Model

- Metadata-rich paths must not recover semantic identity through rendered
  spelling after a complete structured miss.
- Retained raw or no-id compatibility is acceptable only when it has a clear
  owner, limitation, and removal condition.
- Route-local names and final output spelling are not semantic lookup authority
  unless they feed a metadata-rich lookup path.
- Any newly discovered semantic rendered-string authority must become a narrow
  open blocker before backend restart proceeds.

## Execution Rules

- Keep routine findings in `todo.md`.
- Edit the source idea only for durable blocker or closure notes.
- Use new `ideas/open/*.md` files for separate blockers.
- Keep validation evidence in canonical `test_before.log`,
  `test_after.log`, and hook-managed baseline artifacts.
- Treat testcase-shaped fixes, expectation downgrades, and rendered-string
  wrapper renames as reject signals.

## Steps

### Step 1: Reconstruct Closure Ledger

Goal: Build the final frontend-to-BIR closure ledger from existing closed
ideas and resolved blockers.

Actions:
- Read the closed dependency ideas listed above.
- Classify remaining parser, sema, HIR, HIR-to-LIR, LIR, and BIR retained
  string surfaces.
- Record in `todo.md` which surfaces are structured authority, mirrors,
  diagnostics/display, route-local handles, ABI/final spelling, or explicit
  no-metadata compatibility.

Completion check:
- `todo.md` contains a closure ledger that accounts for all source-idea
  surfaces and names any unproven high-risk surface.

### Step 2: Audit Residual High-Risk Paths

Goal: Verify no known metadata-rich frontend-to-BIR path still uses rendered
strings as semantic authority after a complete structured miss.

Actions:
- Reinspect the HIR registry, constructor/member, generated template, generated
  member, HIR-to-LIR call/type metadata, LIR call payload, and BIR lowering
  boundaries.
- Confirm blockers 201 and 202 remain resolved by their closed evidence.
- If a new semantic rendered-string authority path is found, create a narrow
  blocker idea under `ideas/open/` and update `todo.md`; do not hide it inside
  this gate.

Completion check:
- Either no residual blocker remains, or each blocker has a dedicated open
  idea with reviewer reject signals.

### Step 3: Run Milestone Validation

Goal: Produce fresh proof appropriate for a pre-backend-restart closure gate.

Actions:
- Run a fresh build.
- Run focused frontend/HIR/BIR regression subsets selected by the supervisor.
- Run broader milestone validation or use an accepted hook-managed full-suite
  baseline when that is the repo policy for this checkpoint.
- Compare matching before/after logs with the regression guard.

Completion check:
- `todo.md` records the exact validation commands, log paths, pass counts, and
  regression-guard result, or records the concrete failure that blocks closure.

### Step 4: Closure Decision

Goal: Hand the source idea to lifecycle closure only when its acceptance
criteria are satisfied.

Actions:
- Summarize whether backend restart is allowed next.
- If the ledger and validation are green, ask the plan owner to close idea 195.
- If not green, keep idea 195 open and record the remaining blocker or
  validation failure in the correct lifecycle layer.

Completion check:
- The active lifecycle state clearly says one of:
  - close idea 195 and allow backend restart planning next
  - keep idea 195 open with named blockers
  - switch to a newly created blocker idea
