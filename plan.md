# RV64 Branch Same-Block Home/Value Identity Reconciliation Runbook

Status: Active
Source Idea: ideas/open/646_rv64_branch_same_block_home_value_identity_reconciliation.md

## Purpose

Classify and repair the RV64 branch stack-load same-block home/value identity
boundary exposed after prepared branch stack-source freshness is already
selected.

## Goal

Advance one semantic same-block home/value identity shape past
`home_value_mismatch`, or reclassify the representative row to a more precise
owner with current evidence.

## Core Rule

Do not infer value identity from stack offsets, final assembly, source syntax,
local names, or diagnostic strings. RV64 may consume a same-block branch
stack-load home only when explicit prepared facts prove that the selected home
and source value are the same semantic branch operand at the consumer point.

## Read First

- ideas/open/646_rv64_branch_same_block_home_value_identity_reconciliation.md
- ideas/closed/590_branch_stack_load_freshness_contract.md
- ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md
- ideas/closed/636_prepared_branch_stack_source_freshness_publication.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md

## Current Scope

- Refresh diagnostics for the `src/990127-1.c` same-block RHS `%lv.a` row.
- Identify the prepared home, source value, branch block, and consumer point.
- Decide whether the mismatch is missing producer identity, stale or absent
  home-value publication, RV64 consumer key mismatch, or another precise owner.
- Add producer or RV64 consumer support only for explicit home/value identity
  evidence.

## Non-Goals

- Do not reopen prepared branch stack-source freshness publication closed by
  idea 636.
- Do not reopen RV64 terminator-fragment lowering closed by idea 645.
- Do not touch clobber-safety authority, destination fan-in authority,
  parameter ABI/home admission, runtime policy, expectations, unsupported
  markers, allowlists, timeouts, or accounting.
- Do not add named-case handling for `src/990127-1.c`, `%lv.a`, or `block_1`.

## Working Model

The original `src/990127-1.c` `block_1` / `lhs` `%t6` freshness row was moved
past the missing-source-freshness blocker by idea 636. The current first
failure is a separate same-block `role=rhs`, `value=%lv.a` row with
`authority_status=home_value_mismatch`,
`source_freshness_status=missing_value`, and zero freshness candidates.

## Execution Rules

- Keep routine execution notes in `todo.md`.
- Start with probes and diagnostics before changing producer or RV64 code.
- Prefer semantic home/value identity coverage over source-file-specific
  assertions.
- Preserve fail-closed behavior for missing value identity, mismatched homes,
  stale publication, ambiguous candidates, and unrelated branch operand
  shapes.
- Any code-changing slice needs fresh build proof and the supervisor-selected
  regression comparison.

## Steps

### Step 1: Refresh Same-Block Identity Evidence

Goal: confirm the current first owner and exact same-block RHS `%lv.a`
home/value mismatch shape.

Actions:
- Refresh object-route diagnostics for `src/990127-1.c`.
- Capture the relevant BIR and prepared-BIR branch row, including branch block,
  role, source value, selected home, and consumer point.
- Record whether current evidence points to producer identity, publication
  freshness, RV64 consumer lookup, or a different owner.

Completion check:
- `todo.md` records the refreshed first owner, exact same-block row shape, and
  likely ownership boundary.

### Step 2: Locate The Identity Authority Boundary

Goal: determine where explicit home/value identity should be produced or
consumed.

Actions:
- Inspect prepared branch stack-load publication and RV64 consumer lookup for
  the selected row.
- Compare with closed ideas 590, 593, and 636 so this route does not reopen
  generic source freshness.
- Identify the smallest semantic identity fact or consumer key repair that can
  prove the selected home and value match at the branch consumer point.

Completion check:
- `todo.md` identifies the selected repair route or records a precise
  reclassification to another owner.

### Step 3: Implement One Semantic Identity Path

Goal: advance one legal same-block home/value identity shape without weakening
branch authority contracts.

Actions:
- Add producer or RV64 consumer support for the selected explicit identity
  path.
- Keep missing, stale, ambiguous, or mismatched identity states rejected with
  precise diagnostics.
- Add focused positive and negative coverage for the semantic shape, not the
  named representative row.

Completion check:
- The narrow build/test proof passes, and the representative row advances
  past `home_value_mismatch` or the route records why no legal repair is
  available.

### Step 4: Validate Boundaries And Residual Owners

Goal: prove the route is not a testcase-shaped shortcut and does not weaken
branch freshness or clobber-safety contracts.

Actions:
- Re-run the supervisor-selected branch/RV64 object subsets.
- Compare before/after logs with the regression guard.
- Inspect nearby branch stack-load identity cases affected by the shared path.
- Record remaining residual owners in `todo.md`.

Completion check:
- Regression proof is non-regressing, negative identity cases remain rejected,
  and any remaining residuals are assigned to explicit owners.

### Step 5: Final Lifecycle Review

Goal: decide whether the same-block home/value identity idea is complete.

Actions:
- Compare final behavior against the source idea acceptance criteria and
  reviewer reject signals.
- Close only if one semantic same-block identity shape advanced or the
  representative row was reclassified to a more precise owner.

Completion check:
- Plan owner can close, deactivate, or split the lifecycle state with matching
  validation evidence.
