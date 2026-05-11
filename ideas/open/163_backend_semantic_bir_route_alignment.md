# Backend Semantic BIR Route Alignment

Status: Open
Created: 2026-05-11

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

## Reviewer Reject Signals

- A slice only deletes snippets, broadens matching, or marks tests unsupported.
- Expected snippets are updated without proving the actual BIR is semantically
  correct.
- A semantic BIR test is silently converted into an LLVM-output test.
- A raw string fallback overrides a valid `LinkNameId`.
- The final proof accepts the same 31 failures as the new baseline.
