# Current Packet

Status: Active
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Prove Structured Argument Identity

## Just Finished

Step 7 regression fix complete: restored the two full-suite runtime cases
`cpp_positive_sema_template_alias_member_typedef_qualified_carrier_runtime_cpp`
and `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`.
`specialization_type_identity_*` now ignores stale template/deferred carrier
metadata after aliases or member typedefs resolve to non-nominal concrete
types, while still using structured nominal metadata for unresolved typedefs,
template records, structs, unions, and enums.

## Suggested Next

Next coherent packet: supervisor review/acceptance of the Step 7 regression
fix, then decide whether to hand off to plan-owner lifecycle close review for
`ideas/open/149_template_instantiation_structured_argument_key.md`.

## Watchouts

- The fix deliberately does not weaken tests or add named-case matching. It
  keeps equality, ordering, and hashing aligned by gating nominal metadata in
  all three `specialization_type_identity_*` helpers.
- The regression symptom was stale structured carrier metadata on resolved
  primitive aliases: generated `is_same_v<same,same>` constants were false even
  though the displayed concrete types matched.
- `HirRecordOwnerTemplateIdentity::specialization_key` still stores
  `spec_key.canonical` as a serialized compatibility bridge; remove it only
  after HIR record-owner identity can carry structured specialization owner and
  argument data directly.

## Proof

Step 7 proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_positive_sema_template_alias_member_typedef_qualified_carrier_runtime_cpp|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|frontend_hir_lookup_tests|cpp_hir_|frontend_parser_lookup_authority_tests|positive_sema_|frontend_cxx_)'`

Result: build succeeded and 147/147 selected tests passed. Proof log:
`test_after.log`.

Focused repro before the full proof:

`ctest --test-dir build -j --output-on-failure -R '^(cpp_positive_sema_template_alias_member_typedef_qualified_carrier_runtime_cpp|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp)$'`

Result: 2/2 selected tests passed.

Supervisor regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: pass; before had 145/147 passing with the two runtime regressions
failing, after has 147/147 passing with both failures resolved and no new
failures.
