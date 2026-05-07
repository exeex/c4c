# Current Packet

Status: Active
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Remove or Label Remaining String Mirrors

## Just Finished

Completed the Step 5 pending-template-type identity packet. The latest slice
changed `make_pending_template_type_key` to build a structured
`PendingTemplateTypeKey` over pending kind, structured `TypeSpec`, owner
identity, ordered type/NTTP bindings, context, and span. Pending-template-type
dedup and resolved tracking now use structured key sets; the retained rendered
pending-type string is display/compatibility-only.

## Suggested Next

Execute Step 6: Remove or Label Remaining String Mirrors.

Next coherent packet: clean up string helpers and compatibility tests that are
now display-only, without changing pending-template-type semantic identity.
Delete obsolete string-key helpers where semantic callers are gone, or rename
and label surviving rendered fields as display-only or compatibility mirrors
with concrete removal criteria recorded here.

## Watchouts

- `HirRecordOwnerTemplateIdentity::specialization_key` still stores
  `spec_key.canonical` as a metadata bridge; later packets should replace that
  bridge only when record-owner template identity can carry the structured key
  without widening this slice.
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

Supervisor regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: pass; 109 passed before and after, with no new failing tests.
