Status: Active
Source Idea Path: ideas/open/prealloc-runtime-carrier-naming-alignment.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Clarify Runtime Helper Resource Names

# Current Packet

## Just Finished

Completed Step 2 comment/grouping clarification in
`src/backend/prealloc/runtime_helpers.hpp`.

Added sparse comments around `PreparedF128RuntimeHelper::CarrierBinding`,
`PreparedI128RuntimeHelper::LaneBinding`, both `ResourcePolicy` structs, and
both `SelectedCallOwnershipPolicy` structs. The comments make the boundary
explicit: carrier/lane bindings are source/result facts as bound for helper-call
ABI marshaling, resource policies describe helper-call obligations, and selected
call ownership is a helper-call completeness summary.

No public type names, fields, helper behavior, or prepared-printer labels were
changed. Printer labels intentionally stay unchanged because this packet only
clarified existing data-contract comments; it did not rename a public contract.

## Suggested Next

Execute Step 3 with one small comment/grouping repair in
`src/backend/prealloc/special_carriers.hpp`: clarify that special carrier
aggregates are value/fact completeness contracts for later prealloc phases, not
runtime helper-call resource policies. Keep public type names and printer labels
unchanged unless the supervisor explicitly delegates a public contract rename.

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

## Proof

Delegated backend proof passed, 162/162 backend tests:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Proof log: `test_after.log`

Whitespace proof passed: `git diff --check`
