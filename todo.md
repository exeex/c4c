Status: Active
Source Idea Path: ideas/open/prealloc-runtime-carrier-naming-alignment.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Align Printer Mirrors

# Current Packet

## Just Finished

Completed Step 4 printer mirror audit for
`src/backend/prealloc/prepared_printer/runtime_helpers.cpp` and
`src/backend/prealloc/prepared_printer/special_carriers.cpp`.

No printer edits were required. Steps 2 and 3 only clarified comments and
grouping in the runtime-helper and special-carrier headers; they did not rename
public prepared data contracts. The runtime-helper printer already reports
helper-call resources, ABI/marshal ownership, live preservation, clobbers, and
selected call ownership under existing field names. The special-carrier printer
already reports i128 and f128 carrier facts under carrier/fact terminology. No
second taxonomy or stale public label was found.

## Suggested Next

Execute Step 5 closure audit as a no-code packet. Confirm that the source idea's
acceptance criteria were satisfied by comment/grouping clarification plus the
printer mirror audit, and recommend plan-owner closure if no remaining public
rename is needed.

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

## Proof

No code changed, so no `test_after.log` was created.

Whitespace proof passed: `git diff --check`
