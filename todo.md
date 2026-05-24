Status: Active
Source Idea Path: ideas/open/prealloc-runtime-carrier-naming-alignment.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Clarify Special Carrier Fact Names

# Current Packet

## Just Finished

Completed Step 3 comment/grouping clarification in
`src/backend/prealloc/special_carriers.hpp`.

Added sparse comments around the i128, f128, atomic, intrinsic, and inline-asm
carrier aggregate families. The comments make the boundary explicit: special
carriers record value, instruction, operand, and completeness facts consumed by
later prealloc phases, not runtime helper-call resource policies.

No public type names, fields, carrier behavior, support decisions, implementation
files, or prepared-printer labels were changed. Printer labels intentionally stay
unchanged because this packet only clarified existing header comments; it did
not rename a public prepared data contract.

## Suggested Next

Execute Step 4 printer mirror audit as a no-code or comment-only packet. Confirm
that `src/backend/prealloc/prepared_printer/runtime_helpers.cpp` and
`src/backend/prealloc/prepared_printer/special_carriers.cpp` still mirror the
data-contract terminology after the Step 2 and Step 3 comment clarifications,
and leave printed labels unchanged unless a real public contract rename is
delegated.

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

## Proof

Delegated backend proof passed, 162/162 backend tests:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Proof log: `test_after.log`

Whitespace proof passed: `git diff --check`
