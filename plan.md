# AArch64 Scalar Local Storage Writeback Sizing Runbook

Status: Active
Source Idea: ideas/open/339_aarch64_scalar_local_storage_writeback_sizing.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the focused AArch64 local scalar storage/writeback residual split from
the post-338 backend-regex inventory.

## Goal

Make non-address-exposed scalar locals carry usable size/alignment facts and
produce correct AArch64 initialization, writeback, reload, compare, and return
behavior for the `00086` and `00111` owner.

## Core Rule

Fix the semantic local sizing/writeback path. Do not special-case the
c-testsuite filenames, and do not broaden this owner into FP, pointer/null,
return-spill, aggregate, variadic, runner, timeout, expectation, or
registration work.

## Read First

- `ideas/open/339_aarch64_scalar_local_storage_writeback_sizing.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md`
- Current evidence summarized in `todo.md` before this switch:
  - `00086` prepared BIR performs short local update/writeback, but generated
    AArch64 uses frame size 0, reloads `[sp]`, and misses the incremented
    short writeback.
  - `00111` initializes only `long l`, reads short `s` before initialization,
    computes `s - l`, misses writeback to `s`, and returns the stale short
    slot.

## Current Targets

- Representative external tests:
  - `c_testsuite_aarch64_backend_src_00086_c`
  - `c_testsuite_aarch64_backend_src_00111_c`
- Local backend coverage around prepared local slot sizing, scalar
  `store_local`/`load_local`, truncation/promotion, and AArch64 local slot
  writeback.

## Non-Goals

- Do not repair FP comparison result materialization for `00119` or `00123`.
- Do not repair FP expression/call argument materialization for `00174`.
- Do not repair pointer-null conditionals or pointer-local buckets such as
  `00112` or `00144`.
- Do not repair broad return-spill or ABI materialization such as `00200`.
- Do not change expectations, unsupported classifications, allowlists,
  runner behavior, timeout policy, CTest registration, or proof-log policy.
- Do not reopen closed focused owners from pass/fail counts alone.

## Working Model

The current evidence points at scalar local storage facts and AArch64 local
slot writeback, not arithmetic itself. The bad path may be in prepared stack
object/access size publication, lowering handoff, frame-slot allocation, or
machine lowering that consumes local `store_local` and `load_local` records.
The first packet should localize that boundary before changing code.

## Execution Rules

- Keep routine packet progress and proof details in `todo.md`.
- Prefer backend unit coverage for the semantic local-sizing/writeback
  contract before relying on c-testsuite filenames.
- Generated assembly probes are useful only when tied back to a general local
  sizing/writeback rule.
- If the first bad fact moves outside scalar local storage/writeback, record
  the new boundary in `todo.md` and stop for supervisor review.
- Use `test_after.log` as the canonical executor proof log only when the
  supervisor delegates the matching proof command.

## Steps

### Step 1: Localize Scalar Local Slot Sizes And Writeback Path

Goal: identify where the focused short-local cases lose usable local size,
initialization, or writeback facts.

Primary target: prepared stack-object/access facts and AArch64 local
`store_local`/`load_local` lowering for `00086` and `00111`.

Actions:

- Inspect the prepared BIR and backend handoff for `00086` and `00111`.
- Trace scalar local object size/alignment from the source type through the
  prepared stack-object/access representation and into AArch64 frame-slot
  lowering.
- Trace scalar `store_local` writeback for initialization, truncation after
  arithmetic, compound assignment, and final reload/compare/return use.
- Identify whether the first bad boundary is fact production, fact
  normalization, frame-slot allocation, or machine lowering consumption.
- Add or adjust narrow backend coverage only if it proves the general
  local-sizing/writeback contract.

Completion check:

- `todo.md` records the concrete first bad boundary and the narrow proof or
  probe commands used to find it, without implementation drift outside this
  owner.

### Step 2: Repair Scalar Local Sizing And Writeback

Goal: make non-address-exposed scalar locals lower with stable size/alignment
and correct writeback semantics.

Primary target: the producer/consumer boundary localized in Step 1.

Actions:

- Repair the general scalar local sizing/writeback rule at the localized
  boundary.
- Preserve existing aggregate, address-exposed local, pointer, variadic, and
  ABI behavior unless the Step 1 evidence proves shared ownership.
- Keep the change focused on semantic local slot facts and writeback, not on
  emitted instruction spelling for one testcase.
- Extend focused backend coverage for the repaired contract when existing
  tests do not pin it down.

Completion check:

- Focused backend coverage passes and generated assembly for `00086` and
  `00111` no longer shows the old frame-size-zero, uninitialized load, or
  missing writeback failure mode.

### Step 3: Prove Focused Runtime Progress And Reclassify Residuals

Goal: prove the focused owner advanced and identify any remaining first bad
facts before lifecycle closure.

Primary target:
`c_testsuite_aarch64_backend_src_00086_c|c_testsuite_aarch64_backend_src_00111_c`.

Actions:

- Run the supervisor-delegated focused proof command after code changes.
- Confirm `00086` uses the incremented short value for comparison or an
  equivalent register-resident semantic value.
- Confirm `00111` initializes `s`, applies `s -= l`, writes back the updated
  short, and returns the updated value or an equivalent register-resident
  semantic value.
- If either case still fails, classify the new first bad fact without
  expanding this owner by default.
- Record proof results and any residual classification in `todo.md`.

Completion check:

- `todo.md` contains focused proof results, residual classification if needed,
  and enough evidence for the supervisor to decide whether to continue,
  review, or close the focused idea.
