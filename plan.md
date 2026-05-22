# AArch64 Recursive Call Argument Preservation Runbook

Status: Active
Source Idea: ideas/open/349_aarch64_recursive_call_argument_preservation.md
Activated From: parked open idea

## Purpose

Refresh and, if still current, repair AArch64 recursive or nested-call value
preservation where generated code consumes caller-clobbered argument registers
after an intervening `bl`.

## Goal

Make the current recursive-call argument preservation first bad fact advance
through a general AArch64 backend repair, with focused coverage proving live
caller-side values are preserved or reloaded before post-call use.

## Core Rule

Do not continue this route from stale historical `00176` or `00181` notes.
Step 1 must refresh generated-code evidence and must stop for lifecycle
handoff if the current first bad fact is block-label ordering, local/formal
frame-slot publication, stack-preserved pointer formal publication, indexed
aggregate writeback, frame layout, or any owner already split from this source
idea.

## Read First

- `ideas/open/349_aarch64_recursive_call_argument_preservation.md`
- generated artifacts for `c_testsuite_aarch64_backend_src_00176_c`
- generated artifacts for `c_testsuite_aarch64_backend_src_00181_c`
- focused backend tests around AArch64 call argument publication,
  caller-clobbered register preservation, and recursive/nested call paths
- handoff notes for ideas 352, 353, 358, 359, 360, 361, and 362 when evidence
  points outside this source idea

## Current Targets

- AArch64 live-value preservation across nested or recursive `bl`
  instructions.
- Post-call reloads or publications for source values that remain live after a
  call but were previously held only in caller-clobbered argument registers.
- Focused backend coverage for recursive or same-module call shapes where
  later control-flow, array mutation, or another call consumes the original
  argument value.

## Non-Goals

- Do not reopen indexed aggregate address/writeback, block-label emission,
  local/formal frame-slot publication, stack-preserved pointer formal
  publication, scalar formal post-call reloads, variadic/byval/HFA aggregate
  publication, scalar cast publication, return publication, frame-slot layout,
  runner behavior, timeout policy, CTest registration, unsupported
  classification, or proof-log policy without fresh generated-code evidence
  that makes that owner the current first bad fact.
- Do not special-case `00176`, `00181`, `quicksort`, `Hanoi`, one function,
  one argument index, one register pair, one block, or one emitted instruction
  neighborhood.
- Do not claim progress through expectation rewrites, helper renames,
  classification-only changes, or generated-text reshuffling that leaves
  post-call consumers able to read stale caller-clobbered registers.

## Working Model

Idea 349 previously repaired stale cross-block call-argument publication from
callee-saved preservation homes, then parked because the visible representative
failures moved to adjacent owners. The executor must first re-establish
whether a live recursive/nested call value is again the current first bad fact.
If the refreshed evidence belongs to a split owner, record the evidence in
`todo.md` and return for lifecycle routing instead of widening this runbook.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Preserve source-idea intent; do not edit the source idea unless a lifecycle
  handoff or durable source correction is required.
- Add or update focused backend coverage before treating `00176` or `00181`
  as acceptance proof.
- Use a validation ladder appropriate to touched code: build, focused backend
  tests, the refreshed representative subset, then broader backend or
  supervisor-selected regression checks when shared AArch64 call-boundary
  logic changes.

## Ordered Steps

### Step 1: Refresh Current First Bad Fact

Goal: establish whether a current representative failure is still in this
source idea's recursive or nested-call argument preservation scope.

Primary target: generated AArch64 artifacts and focused dumps for
`c_testsuite_aarch64_backend_src_00176_c` and
`c_testsuite_aarch64_backend_src_00181_c`.

Actions:

- Rebuild or regenerate the relevant representative backend artifacts.
- Capture the first observable compile-time or runtime bad fact.
- Map the bad fact to semantic BIR, prepared records, generated storage,
  ABI argument registers, emitted instructions, and the observing post-call
  use.
- If the owner is outside this source idea, update `todo.md` with the evidence
  and stop for lifecycle handoff.

Completion check:

- The first bad fact is either confirmed as in-scope with concrete artifacts,
  or an out-of-scope owner is identified clearly enough for plan-owner routing.

### Step 2: Localize The Preservation Boundary

Goal: identify the exact AArch64 lowering or publication boundary that lets a
post-call consumer read a caller-clobbered argument register instead of the
live source value.

Primary target: prepared live-value homes, caller-save or preserved homes,
post-call reloads, call-argument publication, and selected AArch64 lowering
for recursive or nested calls.

Actions:

- Trace the live value from semantic BIR through prepared storage and selected
  AArch64 lowering.
- Determine whether the fault is missing preserved-home publication, missing
  post-call reload, stale call-argument reuse, caller-save spill selection, or
  instruction ordering around a `bl`.
- Compare nearby working recursive or nested-call cases so the repair target
  is not one named representative.

Completion check:

- The failing owner is stated as a concrete preservation or post-call
  publication boundary with enough detail to write focused backend coverage.

### Step 3: Add Focused Backend Coverage

Goal: prove the localized owner independently from the external c-testsuite
representatives.

Primary target: existing AArch64 backend test surfaces for call argument
publication, recursive calls, same-module calls, and post-call value reuse.

Actions:

- Add or extend focused coverage for a live value that crosses one or more
  calls and is consumed afterward.
- Include adjacent guardrails selected by the supervisor for call publication,
  branch/control, selected-address, and already repaired recursive
  preservation paths when the touched path overlaps them.
- Confirm the focused test fails before or is demonstrably targeted at the
  localized missing capability.

Completion check:

- Focused coverage exercises the general owner and would reject a
  representative-only or register-name-only shortcut.

### Step 4: Repair General AArch64 Preservation

Goal: make post-call consumers observe the original live source value through
the correct preserved home, reload, or publication path.

Primary target: the localized AArch64 backend owner from Step 2.

Actions:

- Implement the smallest general repair at the localized preservation
  boundary.
- Keep adjacent repairs stable rather than reintroducing special publication
  paths.
- Avoid broad rewrites unless localized evidence proves the current
  abstraction cannot represent the required live-value flow.

Completion check:

- The focused backend coverage from Step 3 passes and generated artifacts show
  the corrected preservation or reload before the observing post-call use.

### Step 5: Validate And Classify The Result

Goal: prove the slice and classify any remaining representative failure.

Primary target: focused backend tests, `c_testsuite_aarch64_backend_src_00176_c`,
`c_testsuite_aarch64_backend_src_00181_c`, and supervisor-selected broader
backend guardrails.

Actions:

- Run the delegated build and focused proof command.
- Run the refreshed representative subset or delegated external proof.
- If a representative still fails, classify the new first bad fact against
  this source idea before claiming completion.
- Escalate to broader backend validation when shared call-boundary,
  preservation, or publication logic changed.

Completion check:

- The repaired owner is green under focused proof, adjacent guardrails remain
  stable, and each representative either passes or advances to a freshly
  classified first bad fact for lifecycle handoff.
