# AArch64 Byval Aggregate Call Argument Lane Publication Plan

Status: Active
Source Idea: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md

## Purpose

Repair the resumed AArch64 caller-side byval aggregate lane publication
failure where a small byval aggregate is passed as a temporary address instead
of packed into the AAPCS64 integer argument register lane.

Goal: make small byval aggregate call arguments publish their payload bytes
from prepared storage into the ABI argument lanes before `bl`, while
preserving prior rounded-placement and upper-lane repairs.

Core Rule: repair the general byval aggregate call-argument publication rule;
do not special-case `00204`, `arg`, `fa_s1`, `s1`, one stack offset, one
register, or one emitted call sequence.

## Read First

- `ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md`
- Current generated AArch64 and prepared/BIR evidence for
  `c_testsuite_aarch64_backend_src_00204_c`
- Prior idea 328 lifecycle notes for:
  - single aggregate register-lane publication
  - rounded byval register-slot placement and register-to-stack transition
  - partial upper-lane `%9s` byval aggregate publication

## Current Targets

- Caller-side AArch64 byval aggregate argument lowering for `00204`.
- The current first bad fact: `fa_s1(s1)` receives the prepared byval
  temporary address in `x0` instead of the one-byte payload packed into `w0`.
- Focused backend coverage for small byval aggregate payload publication from
  prepared storage into AAPCS64 integer argument lanes.

## Non-Goals

- Do not reopen fixed-formal entry publication unless fresh evidence shows a
  callee consumes an unpublished fixed formal before first use.
- Do not reopen HFA/floating variadic, stdarg cursor, non-HFA aggregate
  `va_arg`, MOVI, local/value-home, frame/formal, or scalar-cast owners
  without a new first bad fact tying them to this route.
- Do not change expectations, unsupported classifications, allowlists, CTest
  registration, timeout policy, runner behavior, or proof-log policy.
- Do not rewrite AAPCS64 aggregate classification broadly unless the localized
  byval lane evidence requires it.

## Working Model

- Idea 328 previously repaired several byval subcases, but the fresh
  post-347 inventory found a current smaller byval regression or newly exposed
  subcase.
- The selected representative is inside idea 328's source scope because the
  caller reaches a byval aggregate call boundary without publishing payload
  bytes into the integer argument lane the callee reads.
- The first implementation packet should re-localize against current
  generated artifacts before editing, because prior repairs may have changed
  the relevant helper boundaries.

## Execution Rules

- Localize before editing and record the first generated-code fact in
  `todo.md`.
- Preserve stack-passed handling for larger byval aggregates and prior
  register-to-stack transition behavior.
- Prefer semantic publication from prepared aggregate storage over
  testcase-shaped callsite matching.
- Add or update focused backend coverage before treating the representative as
  acceptance proof.
- Use `c_testsuite_aarch64_backend_src_00204_c` as representative proof after
  focused coverage establishes the repaired rule.

## Steps

### Step 1: Relocalize The Current Byval Lane First Bad Fact

Goal: confirm the current `00204` byval lane failure against fresh generated
artifacts.

Primary target: generated AArch64, prepared records, and semantic/prepared
handoff for `c_testsuite_aarch64_backend_src_00204_c`.

Actions:

- Trace `fa_s1(s1)` from semantic byval argument through prepared aggregate
  storage, call-argument lowering, emitted `x0`/`w0`, and callee entry
  consumption.
- Confirm whether the caller passes a temporary address in `x0` rather than
  the one-byte payload in `w0`.
- Check nearby `fa_s2` and prior repaired byval shapes enough to avoid
  regressing already-covered register-lane and rounded-placement behavior.
- Record the first-bad-fact summary in `todo.md`.

Completion check:

- `todo.md` names the current concrete source bytes, prepared storage,
  destination ABI lane, emitted callsite fact, and why the owner is caller-side
  byval aggregate lane publication.

### Step 2: Repair General Small Byval Payload Publication

Goal: publish small byval aggregate payload bytes into AAPCS64 integer
argument lanes before the call.

Primary target: the AArch64 call-argument lowering path identified by Step 1.

Actions:

- Repair the helper or lowering path that chooses address publication instead
  of payload packing for register-passed small byval aggregates.
- Keep the repair general across small byval sizes and source storage forms
  covered by the existing ABI classification.
- Preserve prior behavior for larger stack-passed byval aggregates, rounded
  register-slot consumption, and partial upper-lane publication.

Completion check:

- The localized callsite no longer passes the byval temporary address for a
  register-passed small aggregate, and the diff contains no testcase-shaped
  shortcut or expectation change.

### Step 3: Add Focused Backend Coverage

Goal: pin the repaired byval payload publication contract outside the external
representative.

Primary target: existing AArch64 backend call lowering or prepared handoff
coverage.

Actions:

- Add or extend focused backend coverage for a small byval aggregate whose
  payload must be packed into an integer argument lane before `bl`.
- Include a guard that distinguishes payload publication from passing the
  prepared temporary address.
- Preserve or run adjacent coverage for rounded byval placement and upper-lane
  byval publication when available.

Completion check:

- Focused backend coverage proves payload bytes reach the ABI lane and fails
  or would have failed for address-passing behavior.

### Step 4: Prove Representative Progress

Goal: prove the `00204` representative advances past the current `fa_s1`
byval lane first bad fact.

Primary target: `c_testsuite_aarch64_backend_src_00204_c`.

Actions:

- Run the supervisor-delegated representative proof for `00204`.
- Confirm `fa_s1(s1)` and nearby small byval call outputs advance or pass.
- If `00204` still fails, classify the next first bad fact before claiming
  this idea complete.

Completion check:

- `00204` no longer shows the targeted byval temporary-address call argument
  fault, or `todo.md` records the next distinct first bad fact.

### Step 5: Guard Adjacent Byval And Variadic Repairs

Goal: ensure the repair does not regress prior byval and adjacent variadic
publication owners.

Primary target: focused backend tests and representative guards selected by
the supervisor.

Actions:

- Run the delegated focused backend and representative guard subset.
- Include prior byval placement, upper-lane publication, fixed-formal entry,
  and local/value-home guardrails when the supervisor selects them.
- Treat expectation weakening or newly unsupported tests as route drift.

Completion check:

- Adjacent byval/variadic guardrails remain stable and proof results are
  recorded in `todo.md`.
