# Current Packet

Status: Active
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Prove Structured Argument Identity

## Just Finished

Step 6 complete: remaining string mirrors are deleted, display-named, or
explicitly labeled as compatibility-only. `canonical_type_str` survives only as
a compatibility wrapper over `format_type_for_specialization_display_key`, and
semantic value-argument `is_same` handling now compares structured `TypeSpec`
identity through `specialization_type_identity_equal`.

## Suggested Next

Next coherent packet: execute Step 7 proof of structured argument identity.
Use the supervisor-chosen focused parser/Sema/HIR template argument tests, add
or confirm probes for formatting-stable and ambiguous rendered argument
identity, then run the broader parser/Sema/HIR validation checkpoint required
by `plan.md`.

## Watchouts

- `HirRecordOwnerTemplateIdentity::specialization_key` still stores
  `spec_key.canonical` as a serialized compatibility bridge; remove it only
  after HIR record-owner identity can carry structured specialization owner and
  argument data directly.
- `encode_pending_type_ref` is retained as a compatibility wrapper over
  `format_pending_type_ref_for_display` because existing frontend lookup tests
  still call it directly; it is no longer part of pending-type dedup/resolved
  identity.
- The structured pending key reuses `specialization_type_identity_*` for
  `TypeSpec` equality/hashing; future cleanup should avoid reintroducing
  formatted type strings as semantic keys.

## Proof

Previous Step 6 proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_hir_)'`

Result: 109/109 tests passed. Proof log: `test_after.log`.

Cleanup check:

`rg -n "canonical_type_str" src tests --glob '!build/**'`

Result: only the compatibility wrapper definition remains in
`src/frontend/hir/hir_ir.hpp`.

Supervisor regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: pass; 109 passed before and after, with no new failing tests.
