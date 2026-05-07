# Current Packet

Status: Complete
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Remove or Label Remaining String Mirrors

## Just Finished

Completed the final Step 6 string-mirror cleanup. The remaining semantic
`canonical_type_str` callers in template value-argument `is_same` handling now
use `specialization_type_identity_equal` on structured `TypeSpec` values.
`canonical_type_str` remains only as an explicit compatibility wrapper over
`format_type_for_specialization_display_key`; `make_specialization_key` builds
`SpecializationKey::canonical` through the display-named helper while
structured owner/argument identity remains authoritative. Pending-template
display renderers and the HIR record-owner serialized specialization bridge are
commented as display/compatibility-only mirrors with removal criteria.

## Suggested Next

Next coherent packet: supervisor lifecycle review for Step 6 completion and
commit readiness.

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

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_hir_)'`

Result: 109/109 tests passed. Proof log: `test_after.log`.

Cleanup check:

`rg -n "canonical_type_str" src tests --glob '!build/**'`

Result: only the compatibility wrapper definition remains in
`src/frontend/hir/hir_ir.hpp`.

Supervisor regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: pass; 109 passed before and after, with no new failing tests.
