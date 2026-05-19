# AArch64 C-Testsuite 00205 Value Materialization Residual Runbook

Status: Active
Source Idea: ideas/open/305_aarch64_ctestsuite_00205_value_materialization_residual.md
Switched From: ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md after timeout repair exposed an output mismatch

## Purpose

Classify and repair the now-reachable `00205` output mismatch without
reopening the repaired timeout, sign-extension legality, or test-policy routes.

## Goal

Repair the prepared direct-global load materialization residual so call
arguments consume real scalar values instead of unwritten stack homes, while
making recursive select-chain lowering condition-safe.

## Core Rule

Prepared direct-global load materialization remains the owner. Do not broaden
this into a generic call-retarget or instruction-selection rewrite, and do not
emit recursive select lowering that can leave an outer `csel` observing stale
NZCV flags from a nested producer.

## Read First

- Source idea:
  `ideas/open/305_aarch64_ctestsuite_00205_value_materialization_residual.md`
- Durable route checkpoint: reviewer-derived Step 2 policy is incorporated
  into this runbook through the working model, execution rules, and Step 2.1
  through Step 2.3.
- Parked timeout owner:
  `ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md`
- Accepted timeout result: `00205` completes quickly and fails output
  comparison instead of timing out.
- Accepted legality result: generated assembly contains legal `sxtw x9, w13`,
  not illegal `sxtw w9, w13`.
- Step 1 classification: the high stack-home reads are a prepared
  direct-global load materialization failure. BIR/prepared state preserves the
  global addressing; final AArch64 call consumption falls back to stack homes
  for values that were never written.
- Partial Step 2 evidence: direct call-boundary materialization removed the
  `[sp, #1352]` and `[sp, #1496]` residuals, but deeper right-recursive
  `%t26` and `%t34` select trees still fall back to `[sp, #632]` and
  `[sp, #1064]`.

## Current Targets

- `c_testsuite_aarch64_backend_src_00205_c`
- Prepared direct-global scalar load materialization in the AArch64 backend
- Recursive select/cast/binary producer materialization used before call
  consumption
- Scratch-register, NZCV, ABI-register, and producer-lookup contracts needed
  for condition-safe select-chain lowering

## Non-Goals

- Do not reopen the branch/compare timeout route unless the same timeout
  mechanism returns.
- Do not reopen sign-extension legality unless illegal `sxtw` destination
  width returns.
- Do not claim pass-count progress from classification-only work.
- Do not alter timeout thresholds, runner behavior, stale-process policy,
  expectations, allowlists, unsupported classifications, proof-log policy, or
  CTest registration.
- Do not special-case filename `00205`, exact stack offsets, case indexes,
  temporary labels, or source line numbers.
- Do not borrow unreserved AArch64 registers, rely on call-clobbered ABI
  argument registers as hidden temporaries, or leave a nested select free to
  clobber the condition flags used by an outer `csel`.
- Do not promote function-wide producer lookup into shared machinery without
  documenting or enforcing its dominance/order contract.

## Working Model

The previous failure mode was non-terminating generated control flow caused by
loop-bound compare lowering. The current failure mode is wrong output after
normal completion. Step 1 classified the residual as prepared direct-global
load materialization before call consumption, not a frame-size owner.

The unfinished Step 2 direction is semantic and non-overfit, but it drifted by
combining global-load materialization, recursive producer lowering, select
lowering, call ABI placement, and move retargeting in one patch. The next
route must split that work around an explicit policy:

- emit or recompute the predicate-setting compare immediately before the
  `csel` that consumes it, after any nested value materialization that may
  clobber NZCV;
- materialize nested select operands into reserved scratch or stable temporary
  storage before the outer compare, or reject the local route and request a
  larger owner if the existing scratch model cannot represent the chain;
- treat outgoing ABI argument registers as final destinations, not scratch
  registers that can be clobbered while sibling operands remain live;
- keep direct-global load materialization as the motivating owner and avoid
  turning this into a broad register allocator or ABI rewrite.

## Execution Rules

- Start from the durable Step 2 policy in this runbook and the current
  uncommitted partial Step 2 evidence; the supervisor decides separately
  whether to salvage, replace, or unwind that diff.
- Preserve any semantic direct-global load materialization that remains valid
  after the NZCV/scratch/ABI policy is enforced.
- Keep probes narrow and tied to prepared global load values consumed through
  selects at call boundaries.
- Preserve idea 303 sign-extension legality and idea 304 timeout repair.
- If the select-chain policy requires machinery beyond reserved scratches and
  local producer materialization, stop and request lifecycle split instead of
  hiding that need in another call-boundary patch.
- Record fresh build proof and the supervisor-selected focused subset in
  `todo.md`.

## Steps

### Step 1: Classify Mismatch Mechanism

Goal: identify why generated `00205` reads or prints wrong case-field values
after the timeout path is repaired.

Primary target: generated assembly, frame layout, and value flow for focused
`00205`

Actions:

- Inspect the generated assembly around the case aggregate initialization,
  address formation, loads, stores, and printed field values.
- Explain why offsets such as `[sp, #632]`, `[sp, #1064]`, and `[sp, #1496]`
  are emitted despite a `sub sp, sp, #48` prologue, or replace that hypothesis
  with a better concrete mechanism.
- Run bounded probes only as needed to determine whether the cause is
  frame-size accounting, aggregate/local materialization, address calculation,
  spill/reload, value-copy lowering, or another backend semantic.
- Verify generated assembly still uses legal sign-extension spelling and still
  emits conditional loop-header compares.

Completion check:

- `todo.md` records the concrete output-mismatch mechanism or records why the
  evidence requires a different split owner.

### Step 2.1: Fence The Partial Route And Select Policy

Goal: convert the reviewed partial Step 2 route into a safe executable policy
before further implementation.

Primary target: AArch64 scalar materialization helpers used before call
consumption

Actions:

- Preserve the classification that prepared direct-global load
  materialization is the owner.
- Identify which pieces of the current partial route are salvageable direct
  global-load materialization and which pieces are unsafe broad
  call-retargeting.
- Write or update focused backend coverage for the NZCV hazard if the next
  executor keeps recursive select lowering.
- Define the local invariant for select lowering: nested value materialization
  must happen before the outer predicate compare, or the outer compare must be
  recomputed immediately before the outer `csel`.
- Define the scratch policy: use only reserved backend scratch registers or
  explicit stable temporary storage; if two scratches cannot carry the chain,
  stop and request a split instead of borrowing unreserved registers.
- Define the ABI policy: outgoing call argument registers are final
  destinations and must not be used as hidden temporaries across sibling
  materialization.
- Define the producer-lookup policy: any function-wide producer lookup must be
  dominance/order-safe or stay private to the focused direct-global
  materialization owner.

Completion check:

- `todo.md` names the salvage/replacement decision for the current partial
  implementation diff and the next executor packet has an explicit NZCV,
  scratch, ABI, and producer-lookup policy.

### Step 2.2: Repair Condition-Safe Select-Chain Materialization

Goal: repair the `%t26` and `%t34` residuals without stale NZCV flags,
unreserved-register borrowing, or ABI-register clobbering.

Primary target: prepared direct-global load materialization through recursive
select/cast/binary producers consumed by calls

Actions:

- Materialize direct-global load sources through machine-node memory operands
  only when the source value is known to be a valid prepared global load.
- Lower nested select operands in an order that preserves the predicate flags
  for each `csel`.
- Ensure the final call-bound ABI register receives the selected value only
  after all nested producer work that could clobber it or NZCV is complete.
- Keep helper exposure scoped to a deliberate scalar materialization facility
  with documented clobber/flag semantics.
- Add or update focused backend unit coverage for nested select/global-load
  call-argument materialization when shared lowering code changes.

Completion check:

- Generated assembly no longer consumes unwritten stack homes for the
  classified `%t26`, `%t34`, `%t42`, and `%t50` direct-global load values, and
  nested select lowering cannot observe stale predicate flags.

### Step 2.3: Focused Proof And Broader Safety Check

Goal: prove the repaired direct-global select-chain route without claiming
unrelated pass-count progress.

Primary target: focused c-testsuite subset plus nearby backend coverage

Actions:

- Build the default preset.
- Run the supervisor-selected focused subset covering `00064`, `00139`, and
  `00205`.
- Run targeted backend unit coverage for select/global-load/call-argument
  materialization if Step 2.2 touched shared lowering helpers.
- Inspect final assembly for preserved legal sign-extension spelling, preserved
  conditional loop-header compares, absence of the classified unwritten
  stack-home call reads, and no stale-NZCV select sequence.

Completion check:

- `todo.md` records fresh proof that `00064` and `00139` still pass, `00205`
  no longer fails for the classified value-materialization reason, and any
  later residual is named without silently expanding this owner.

### Step 3: Residual Handoff Or Closure Recommendation

Goal: separate this owner's result from any later residual exposed by the same
testcase.

Primary target: focused proof results after repair or split

Actions:

- Preserve the sign-extension legality result and timeout-repair result in the
  proof notes.
- Record any later residual separately without claiming unrelated pass-count
  progress.
- If the value-materialization owner is complete but the testcase still fails
  for a new reason, request lifecycle handoff with concrete evidence.

Completion check:

- `todo.md` records proof for the value-materialization classification/repair
  and names any remaining residual bucket without expanding this owner
  silently.
