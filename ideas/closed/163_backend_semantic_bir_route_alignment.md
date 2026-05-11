# Backend Semantic BIR Route Alignment

Status: Closed
Created: 2026-05-11
Closed: 2026-05-11

Parent Ideas:
- `ideas/closed/162_link_name_id_backend_symbol_authority.md`
- `ideas/closed/161_hir_template_binding_domain_key_authority.md`

## Goal

Repair the backend semantic BIR route tests after the `LinkNameId` authority
cleanup.

The current `test_baseline.log` for `f05cc432f Close LinkNameId backend symbol
authority idea` records 31 backend route failures. These should be driven back
to green by aligning BIR producers, route harnesses, and expected semantic BIR
snippets with the new structured symbol route. Raw expected snippets may be
updated only when the actual BIR is semantically correct under the new
`LinkNameId` model.

## Why This Idea Exists

Idea 162 intentionally accepted a non-regressing full-suite baseline:
3104 passed, 31 failed, 3135 total before and after. The failures are all
backend route semantic BIR observations, and the sampled failures show multiple
classes:

- stale temp-number snippets, such as expecting `bir.ret i32 %t9` when the
  semantic BIR now returns `%t3`
- route/harness mismatches where a semantic BIR test receives LLVM IR output
  instead of BIR output
- inline asm output constraints that changed shape, such as `+r` versus
  `=r,0`
- aggregate/global/memcpy/memset/dynamic-member-array snippets that likely no
  longer match the current BIR lowering shape

This should not be handled by weakening snippets or marking tests unsupported.
Each failure needs a semantic classification first: expected update, route
harness fix, or real backend lowering bug.

## In Scope

- Inventory the 31 failures in `test_baseline.log` and classify them by failure
  family.
- For each failed test, inspect actual backend route output and determine
  whether it is semantically correct under the new `LinkNameId` authority
  model.
- Update semantic BIR expected snippets only when the actual BIR is correct and
  the old snippet is stale.
- Fix backend route harness selection when a semantic BIR observation test is
  receiving LLVM IR instead of BIR.
- Fix BIR/LIR/backend lowering when actual output is semantically wrong, loses
  `LinkNameId`, or falls back to raw string authority incorrectly.
- Preserve tests for aggregate/byval, function pointer, global struct/array,
  inline asm, builtin memcpy/memset, and dynamic member-array routes.
- Add or update focused tests proving `LinkNameId` transport is still present
  on affected direct-call/global/initializer paths.
- End with a full-suite baseline that no longer carries these 31 backend route
  failures, unless a specific residual failure is split into a new open idea
  with explicit justification.

## Initial Failure Families

- Aggregate/byval/function pointer routes:
  `93`, `96`, `100`, `102`, `161`
- Global struct/array read/write routes:
  `105`, `106`, `107`, `108`
- Inline asm readwrite routes:
  `134`, `136`
- Builtin memcpy/memset aggregate and array routes:
  `139`, `140`, `141`, `142`, `143`, `145`, `147`, `149`,
  `152`, `153`, `154`, `155`
- Nested pointer/member/dynamic array routes:
  `162`, `163`, `164`, `165`, `166`, `167`, `168`, `169`

## Out Of Scope

- Reopening parser/sema/HIR string-table authority decisions from ideas
  158-162.
- Weakening snippets so tests pass without preserving the semantic observation
  intent.
- Marking supported backend route cases unsupported.
- Replacing semantic BIR tests with LLVM text tests.
- Large backend feature work unrelated to the failed BIR route families.

## Acceptance Criteria

- The 31 baseline backend route failures are classified and recorded in the
  runbook/todo.
- Every expected update is justified by actual semantically correct BIR.
- Every route producing LLVM IR for a semantic BIR test is fixed at the harness
  or route-selection layer.
- Any true backend lowering bug is fixed rather than papered over by snippets.
- `LinkNameId` remains authoritative for covered symbol paths after the test
  alignment.
- The relevant backend route subset passes, and final validation updates the
  full-suite baseline to remove these failures.

## Completion Summary

The active runbook completed and the source idea is closed. The route restored
all backend semantic BIR observations accepted from idea 162 without weakening
the semantic observation contract, marking supported cases unsupported, or
converting semantic BIR tests into LLVM-output tests.

The Step 1 inventory classified the 31 baseline failures as follows:

- Aggregate/byval/function pointer routes: tests `93`, `96`, `100`, `102`,
  and `161`.
- Global struct/array read/write routes: tests `105`, `106`, `107`, and
  `108`.
- Inline asm readwrite routes: tests `134` and `136`.
- Builtin memcpy/memset aggregate and array routes: tests `139`, `140`, `141`,
  `142`, `143`, `145`, `147`, `149`, `152`, `153`, `154`, and `155`.
- Nested pointer/member/dynamic array routes: tests `162`, `163`, `164`,
  `165`, `166`, `167`, `168`, and `169`.

The route/lowering repairs generalized aggregate return and byval handling,
global aggregate array addressing, builtin memory route lowering, and dynamic
local aggregate array access through semantic BIR lowering paths rather than
named-test shortcuts. Snippet updates were limited to semantically correct BIR
drift, including temp-number churn and inline asm readwrite constraint shape
changes, while retaining required loads, stores, calls, offsets, builtin
lowering, direct-call/global symbol coverage, and forbidden LLVM-output guards.

Close-time proof:

- Final execution slice through commit `420f0af8e` resolved residual dynamic
  semantic BIR route tests `163` through `168`.
- The executor's delegated narrow residual route command for tests `163`
  through `168` passed all six tests after the snippet update.
- Close-scope guard passed with accepted `test_baseline.log` versus
  `test_after.log`: before `3129/6/3135`, after `3135/0/3135`, resolving the
  remaining six backend semantic BIR failures and introducing no new failures.
- The latest full-suite validation in `test_after.log` reports `100% tests
  passed, 0 tests failed out of 3135`.

No residual backend semantic BIR route failure family was split into a new
open idea.

## Reviewer Reject Signals

- A slice only deletes snippets, broadens matching, or marks tests unsupported.
- Expected snippets are updated without proving the actual BIR is semantically
  correct.
- A semantic BIR test is silently converted into an LLVM-output test.
- A raw string fallback overrides a valid `LinkNameId`.
- The final proof accepts the same 31 failures as the new baseline.
