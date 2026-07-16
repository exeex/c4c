# LIR Aggregate-Owner Module-Owner Canonicalization Blocker

Status: Open
Type: bounded native LIR aggregate-owner provenance repair
Blocked Parent: `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`, Step 4 comparable full-suite proof

## Goal

Restore the native module-owner canonicalization relation required by
`lir_owned_type_spec` when lowering LIR-owned aggregate function types, so the
same valid aggregate key resolves to its matching module tag across the
observed C and C++ lowering paths.

## Why This Exists

831's rejected comparable full-suite gate has 516 new failures across seven
suite categories. Each first stops at `lir_owned_type_spec` in
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp:106-108` with `LIR-owned aggregate
function type requires a matching module owner`. `git blame` attributes those
lines to `b556c6c9f`. The focused `frontend_hir_tests` crash guard for closed
832 passes, and none of the failures use closed 833's
truthiness-LHS-parameter-authority route. This is a broad native
module-owner/provenance contract exposed by the earlier crash repair, not a
reopening of either closed successor.

## In Scope

- Diagnose the missing relation between the structured aggregate key retained
  by `lir_owned_type_spec` and the module tag consulted by
  `find_struct_def_tag_by_owner`.
- Implement the smallest native ownership/canonicalization repair at that
  relation, preserving valid aggregate type identity across representative
  C and C++ lowering paths.
- Add nearby same-feature coverage from more than one affected suite category
  and retain malformed/missing-owner rejection where the contract requires it.
- Obtain focused proof, then provide the evidence 831 needs to retry its
  unchanged Step 4 comparable full-suite gate.

## Out Of Scope

- Reopening or modifying closed 832's bounded `frontend_hir_tests` crash
  contract or closed 833's truthiness-LHS authority contract.
- 831's baseline comparison/acceptance decision, 830 direct-call work, 829
  authority work, Raw-BIR, generic-call work, or test-harness changes.
- Test expectation changes, unsupported markers, allowlists, test filtering,
  rendered-text matching, or weaker diagnostics as a substitute for native
  module-owner repair.

## Acceptance Criteria

1. A first packet identifies why a valid LIR aggregate key cannot find its
   matching module tag, using representative failures from at least two suite
   categories without testcase-specific branching.
2. The repair restores the native key-to-module-owner canonicalization at the
   owning `lir_owned_type_spec` lookup relation while preserving invalid or
   genuinely ownerless input rejection.
3. Nearby same-feature focused coverage from more than one affected category
   passes, along with a fresh build and the relevant malformed-owner guard.
4. The supervisor can use the accepted bounded evidence to return to 831 Step
   4 for the comparable full-suite proof; this idea itself does not claim
   baseline clearance or return 830.

## Reviewer Reject Signals

- Reject a named-test, suite-name, rendered-diagnostic, or category-specific
  exception instead of restoring the key-to-module-owner relation.
- Reject expectation downgrades, unsupported markers, allowlists, filtering,
  or harness changes claimed as canonicalization progress.
- Reject a broad HIR, aggregate metadata, or module-table rewrite that is not
  necessary to repair the `lir_owned_type_spec` owner lookup.
- Reject reopening 832 or 833, or attributing this regression to either closed
  contract without direct first-owner evidence.
- Reject focused-only proof as comparable full-suite clearance, or a return to
  830 before 831 accepts its unchanged Step 4 gate.
