Status: Active
Source Idea Path: ideas/open/prealloc-runtime-carrier-naming-alignment.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Closure Audit

# Current Packet

## Just Finished

Completed Step 5 closure audit for
`ideas/open/prealloc-runtime-carrier-naming-alignment.md`.

The accumulated source changes are limited to comment/grouping clarification in
`src/backend/prealloc/runtime_helpers.hpp` and
`src/backend/prealloc/special_carriers.hpp`. Runtime-helper comments now call
out helper-call marshal plans, helper-call resource obligations, and selected
call/ABI ownership. Special-carrier comments now call out value, instruction,
operand, and completeness facts consumed by later prealloc phases.

No helper-call selection, marshal behavior, carrier requirements, intrinsic
support decisions, atomics, inline-asm behavior, tests, or prepared dump meaning
changed. `src/backend/prealloc/prepared_printer/runtime_helpers.cpp` and
`src/backend/prealloc/prepared_printer/special_carriers.cpp` were audited and
intentionally left unchanged because no public prepared data contract or printed
field meaning was renamed. The printer files still mirror the data contracts
without introducing a second ownership taxonomy.

## Suggested Next

Ask the plan owner to close the active plan. The source idea's acceptance
criteria are satisfied by the comment/grouping clarification plus the printer
mirror audit, and no narrower follow-up idea is required for this naming slice.

## Watchouts

- Keep runtime helper-call resources separate from special carrier facts.
- Do not change helper selection, marshal behavior, carrier requirements,
  intrinsic support decisions, atomics, inline asm, or dump meaning.
- Avoid renaming prepared dump labels; printer alignment should wait until a
  public contract rename actually happens.
- Do not treat atomics, intrinsics, or inline asm as runtime helpers. Their
  `carrier_kind` fields are fact-completeness flags, not helper-call resource
  policies.
- `intrinsic_carriers` references `prepared_call_plan` and `source_callee`
  because intrinsic validation depends on existing call-plan facts; this should
  stay separate from runtime-helper callee/resource ownership.
- Step 3 deliberately avoided public renames; Step 4 should not invent printer
  terminology churn just to mirror comments.
- Step 4 deliberately kept dump labels unchanged because printed dump meaning
  and public prepared data names did not change.
- Any future public rename of runtime-helper resources or carrier facts should
  be a separate plan that updates matching prepared-printer labels in the same
  slice.

## Proof

No code changed in this Step 5 audit, so no `test_after.log` was created.

Whitespace proof passed: `git diff --check`
