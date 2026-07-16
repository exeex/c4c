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

## Accepted Step 1 Diagnosis

Representative C `tests/c/external/gcc_torture/src/pr51323.c` and C++
`tests/cpp/internal/postive_case/runtime/operator_struct_byval_param.cpp` both
reach `lir_owned_type_spec` and fail its matching-module-owner diagnostic. The
function clears `record_def`, qualifier segments/text IDs/count before
`typespec_aggregate_owner_key(type, mod)` and `find_struct_def_tag_by_owner`.
This loses parser-backed declaration/namespace canonicalization, creating a
noncanonical key and valid module-tag miss. The bounded repair must derive or
validate the module-canonical `HirRecordOwnerKey` and resolve/inter its LIR
module tag while metadata remains available, then clear parser-owned carriers.
No HIR owner-index rewrite is in scope.

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

## Resumption Record: Step 2 resumed after upstream durable-owner carrier

Status: resumed after accepted prerequisite
`ideas/closed/835_hir_durable_aggregate_owner_identity_carrier_prerequisite.md`.

- Last accepted progress: Step 1 — **Diagnose the aggregate key/module-tag
  ownership mismatch** is complete. Cleanup-before-owner lookup loses the
  parser-backed declaration/namespace canonicalization required for a valid
  aggregate key to find its matching module tag.
- Interrupted step: Step 2 — **Repair the native owner canonicalization
  relation**.
- Blocker and scope boundary: Step 2 explored both safe LIR-local orderings.
  Sanitizing parser carriers before lookup passes `frontend_hir_tests` and the
  malformed-owner rejection guard, but valid C `pr51323.c` and C++
  `operator_struct_byval_param.cpp` still fail matching module-owner lookup
  (focused CTest failures 163, 164, and 169). Resolving before cleanup restores
  valid C/C++ lookup but segfaults `frontend_hir_tests`; GDB's first fault is
  `typespec_aggregate_owner_key` reading a stale `record_def` during normal
  test `test_hir_to_lir_object_helper_callees_prefer_link_name_ids`. LIR cannot
  safely reconstruct parser-table `TextId` spelling or identity once HIR has
  materialized and parser storage has died.
- Accepted proof references: the safe sanitized variant had a passing fresh
  `cmake --build --preset default` and passing `frontend_hir_tests`/malformed
  rejection guard; the focused CTest returned failures 163, 164, and 169 for
  the still-valid owner paths. The pre-cleanup ordering restored those valid
  paths but segfaulted the HIR guard. No accepted code, implementation commit,
  or canonical regression log exists.
- Prerequisite disposition: 835 is capability complete. Its accepted
  implementation commit is `932c3339b` (`hir: retain canonical aggregate owner
  identity`), and its accepted Step 3 proof is the fresh command
  `cmake --preset default && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|cpp_positive_sema_operator_struct_byval_param_cpp|llvm_gcc_c_torture_src_pr51323_c)$'`;
  matching regression guard evidence recorded 39/42 before and 42/42 after,
  with no new failures.
- Exact return point: resume 834 at Step 2. Implement the LIR lookup against
  835's durable carrier, then execute Step 3 focused C/C++ proof plus the
  malformed guard. Do not return to 831 or claim baseline clearance first.
