# AArch64 Variadic Stdarg Cursor Format Residual Runbook

Status: Active
Source Idea: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Switched From: ideas/open/326_aarch64_variadic_hfa_floating_residual.md

## Purpose

Resume the AArch64 `00204.c` stdarg cursor/format owner after fresh evidence
showed the current first bad fact is format-byte/NUL-test materialization in
`myprintf`, not an HFA register-save-area defect.

## Goal

Classify and repair the AArch64 byte-sized format cursor path used by
`myprintf` so `for (s = format; *s; s++)` stops at the terminating NUL instead
of overreading into adjacent literals and later HFA text.

## Core Rule

Repair a general AArch64 stdarg cursor/format materialization capability. Do
not continue under HFA register-save-area assumptions unless fresh proof first
reaches a standalone HFA stdarg call as the true first bad fact.

## Read First

- `ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md`
- `todo.md`
- current generated AArch64 for `tests/c/external/c-testsuite/src/00204.c`
- `test_after.log` for `c_testsuite_aarch64_backend_src_00204_c`, when the
  supervisor includes it in an executor packet
- AArch64 generated `myprintf` loop around the second stdarg `%7s %9s ...`
  invocation
- AArch64 scalar byte load/store, comparison, branch-condition, and local
  value materialization helpers before editing

## Current Scope

- AArch64 byte/char local materialization for format-cursor loads in
  `myprintf`
- NUL-test comparison and branch-condition publication for loaded format bytes
- stdarg format traversal state in generated AArch64 for `00204.c`
- focused backend coverage for byte-width load, store, compare, and call-loop
  materialization when the localized owner is repaired

## Non-Goals

- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.
- Do not special-case `00204.c`, `myprintf`, one format literal, `.str51`,
  `.str52`, one `%7s` / `%9s` occurrence, one stack offset, one register, one
  emitted `strb`/wide-load sequence, or one printed output substring.
- Do not reopen HFA register-save-area copying, HFA lane publication,
  byval aggregate register-lane allocation, non-HFA aggregate `va_arg`,
  fixed-formal entry publication, local/value-home publication, MOVI
  zero-extension, or f128 transport without direct first-bad-fact evidence.

## Working Model

The current `00204.c` evidence does not cleanly reach the later standalone
`myprintf("%hfa34 %hfa34 %hfa34 %hfa34", ...)` invocation. Generated-code and
debugger evidence instead show the second stdarg `%7s %9s ...` `myprintf`
loop storing the current format byte with `strb`, then reloading an 8-byte
value from the same stack address before comparing against zero. That stale
wide reload keeps the loop alive past the format string terminator, so output
walks into adjacent literals and visibly includes `HFA long double:` text
inside the same invocation.

Treat the visible HFA text as downstream overread evidence until the
format-byte/NUL-test owner is repaired or disproven.

## Execution Rules

- Start from the first bad generated-code fact in `myprintf`'s format loop.
- Prove whether the byte value used for `*s` is loaded, stored, extended, and
  compared with the correct width.
- Preserve nearby stdarg payload, byval aggregate, HFA/floating, fixed-formal,
  and local/value-home repairs.
- Prefer focused backend tests for byte-width materialization and branch
  publication before relying on external c-testsuite proof.
- Stop for lifecycle handoff if the first bad fact moves back to HFA,
  byval-lane, aggregate `va_arg`, MOVI, or another open owner.

## Steps

### Step 1: Localize Format-Byte NUL-Test Materialization

Goal: identify the exact backend boundary that widens or republishes the
format byte used by `myprintf`'s loop condition.

Primary target: generated AArch64 for `00204.c` `myprintf`, focused dumps for
the loop body, and the current `test_after.log` proof when provided.

Actions:

- Locate the second stdarg `%7s %9s ...` `myprintf` invocation and the loop
  over `format`.
- Trace the source-level `*s` byte through generated stack homes, loads,
  stores, comparisons, and branch-condition publication.
- Name the first instruction or prepared value where byte width is lost,
  stale higher bytes are observed, or the NUL comparison consumes the wrong
  materialized value.
- Reject HFA register-save-area ownership unless evidence shows execution has
  reached the standalone HFA stdarg call before the overread.

Completion check:

- `todo.md` names the first bad fact, the owning backend boundary, and whether
  focused coverage is needed before repair.

### Step 2: Add Focused Coverage For Byte Cursor Materialization

Goal: lock the localized stdarg cursor/format owner to a focused backend
contract before implementation.

Primary target: the smallest backend/unit coverage that can assert byte-width
materialization, NUL-test comparison, or branch-condition publication.

Actions:

- Add or extend focused coverage for byte loads/stores and zero/NUL tests
  through the AArch64 path identified in Step 1.
- Include a guard that rejects reloading a byte stack home as a wider value
  when the high bytes are not defined.
- Keep `00204.c` as representative integration proof, not the only proof of
  the repair.

Completion check:

- Focused coverage fails before the repair or directly asserts the previously
  missing byte-width/NUL-test facts.

### Step 3: Repair General AArch64 Byte/NUL Publication

Goal: make the AArch64 backend materialize and test format cursor bytes with
the correct width.

Primary target: the byte load/store, scalar materialization, compare, or
branch-condition publication owner identified in Step 1.

Actions:

- Implement the smallest general repair for the localized byte-width or
  NUL-test materialization defect.
- Preserve existing scalar GPR/FPR, byval aggregate, stdarg cursor,
  local/value-home, HFA, f128, and MOVI behavior.
- Avoid named-case shortcuts or emitted-text matching.
- Run build proof before focused backend and representative proof.

Completion check:

- Focused coverage passes, and the generated `myprintf` loop cannot overread
  past the terminating NUL through stale-wide byte materialization.

### Step 4: Prove Representative And Classify Residuals

Goal: prove the repair on `00204.c` and classify the next first bad fact.

Primary target: `c_testsuite_aarch64_backend_src_00204_c` and the focused
backend tests added in Step 2.

Actions:

- Run the supervisor-delegated proof command for focused tests and the
  `00204.c` representative.
- If the representative reaches the standalone HFA stdarg call and fails
  there, hand off to idea 326 instead of expanding this plan.
- If the representative advances to byval lane, aggregate `va_arg`, MOVI, or
  another known owner, record the handoff in `todo.md` and stop for lifecycle
  routing.
- Ask the supervisor whether broader backend-regex or regression-guard proof
  is needed before closure.

Completion check:

- The representative either passes under this owner or advances to a new,
  explicitly classified first bad fact with lifecycle-ready handoff notes.
