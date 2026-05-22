# AArch64 Variadic HFA And Floating Residual Runbook

Status: Active
Source Idea: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Activated From: parked open idea

## Purpose

Classify and repair the active AArch64 variadic HFA/floating residual without
reopening adjacent owners that have already been split or closed.

## Goal

Make the current `00204.c` HFA/floating or composite variadic call-boundary
first bad fact advance through a general AArch64 backend repair, with focused
coverage proving the repaired owner before relying on the external
c-testsuite representative.

## Core Rule

Do not continue this route on stale historical failures. Step 1 must refresh
the current generated-code evidence and must stop for lifecycle handoff if the
first bad fact is stdarg format traversal, MOVI zero-extension, fixed-formal
entry publication, byval lane placement, or any other owner already split out
of this source idea.

## Read First

- `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`
- generated artifacts for `c_testsuite_aarch64_backend_src_00204_c`
- focused backend tests covering AArch64 variadic, HFA/floating, aggregate
  argument, and call-boundary publication behavior
- recent closed handoffs for adjacent `00204.c` work when evidence points
  outside this idea

## Current Targets

- AArch64 variadic HFA/floating argument publication and loading.
- Composite or variadic call-boundary move preparation when the generated
  failure requires structured scalar FPR, GPR, f128, or q-register authority.
- Focused backend coverage that proves the repaired owner independent of the
  external `00204.c` representative.

## Non-Goals

- Do not reopen local/value-home publication, fixed-formal entry publication,
  byval aggregate lane publication, stdarg cursor or format-byte traversal,
  MOVI zero-extension, F128 memory transport, frame-size/layout, runner
  behavior, timeout policy, CTest registration, unsupported classification, or
  proof-log policy without fresh generated-code evidence that makes that owner
  the current first bad fact.
- Do not special-case `00204.c`, `myprintf`, `fa_hfa11`, one HFA shape, one
  float literal, one register lane, one stack offset, one format string, or
  one emitted instruction sequence.
- Do not claim progress through expectation rewrites, helper renames, or
  classification-only changes.

## Working Model

Previous lifecycle handoffs show this source idea has repeatedly exposed
adjacent `00204.c` owners. The executor must re-establish the current failing
boundary from fresh generated artifacts, then repair only the in-scope owner.
If fresh evidence again proves an adjacent first bad fact, record that in
`todo.md` and return for lifecycle routing instead of widening this runbook.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Preserve source-idea intent; do not edit the source idea unless a lifecycle
  handoff or durable source correction is required.
- Add focused backend coverage before treating the external representative as
  acceptance proof.
- Use a validation ladder appropriate to the touched code: build, focused
  backend tests, the `00204.c` representative, then broader backend or
  supervisor-selected regression checks when the blast radius crosses shared
  AArch64 call-boundary logic.

## Ordered Steps

### Step 1: Refresh Current First Bad Fact

Goal: establish whether the current `00204.c` failure is still in this source
idea's HFA/floating or composite variadic call-boundary scope.

Primary target: generated AArch64 artifacts and focused dumps for
`c_testsuite_aarch64_backend_src_00204_c`.

Actions:

- Rebuild or regenerate the relevant `00204.c` backend artifacts.
- Capture the first observable compile-time or runtime bad fact.
- Map the bad fact to semantic BIR, prepared records, generated source
  storage, ABI argument registers, emitted instructions, and the observing
  call or comparison.
- If the owner is outside this source idea, update `todo.md` with the evidence
  and stop for lifecycle handoff.

Completion check:

- The first bad fact is either confirmed as in-scope with concrete artifacts,
  or an out-of-scope owner is identified clearly enough for plan-owner routing.

### Step 2: Localize The In-Scope Owner

Goal: identify the exact AArch64 lowering boundary that loses or mispublishes
the HFA/floating or composite variadic value.

Primary target: AArch64 call-boundary preparation, HFA lane publication,
floating register-save or overflow-area handling, and structured f128/q
authority paths reached by the refreshed evidence.

Actions:

- Trace the value from semantic BIR through prepared storage and selected
  AArch64 lowering.
- Determine whether the fault is call-lane assignment, lane materialization,
  register-save-area addressing, overflow-area progression, structured
  authority publication, or generated instruction ordering.
- Compare nearby working HFA/floating or aggregate cases so the repair target
  is not one named representative.

Completion check:

- The failing owner is stated as a concrete lowering/publication boundary with
  enough detail to write a focused backend test.

### Step 3: Add Focused Backend Coverage

Goal: prove the localized owner independently from the external c-testsuite
representative.

Primary target: existing AArch64 backend test surfaces for variadic,
HFA/floating, aggregate, or call-boundary publication behavior.

Actions:

- Add or extend focused coverage for the localized value-flow shape.
- Include adjacent guardrails that protect prior local/value-home, fixed
  formal, byval aggregate, stdarg cursor, MOVI, and frame/formal repairs when
  the touched path overlaps them.
- Confirm the focused test fails before or is demonstrably targeted at the
  localized missing capability.

Completion check:

- Focused coverage exercises the general owner and would reject a
  `00204.c`-only or lane-name-only shortcut.

### Step 4: Repair General AArch64 Lowering

Goal: publish or materialize the in-scope value through the correct ABI
storage without testcase-shaped matching.

Primary target: the localized AArch64 backend owner from Step 2.

Actions:

- Implement the smallest general repair at the localized lowering boundary.
- Keep existing adjacent repairs stable rather than reintroducing special
  publication paths.
- Avoid broad rewrites unless the localized evidence proves the existing owner
  abstraction cannot represent the required value flow.

Completion check:

- The focused backend coverage from Step 3 passes and generated artifacts show
  the corrected value publication or materialization before the observing use.

### Step 5: Validate And Classify The Result

Goal: prove the slice and classify any remaining representative failure.

Primary target: focused backend tests, `c_testsuite_aarch64_backend_src_00204_c`,
and supervisor-selected broader backend guardrails.

Actions:

- Run the delegated build and focused proof command.
- Run the `00204.c` representative or the delegated external proof.
- If the representative still fails, classify the new first bad fact against
  this source idea before claiming completion.
- Escalate to broader backend validation when shared call-boundary or ABI logic
  changed.

Completion check:

- The repaired owner is green under focused proof, adjacent guardrails remain
  stable, and `00204.c` either passes or advances to a freshly classified first
  bad fact for lifecycle handoff.
