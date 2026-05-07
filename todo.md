# Current Packet

Status: Active
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Prove Structured Argument Identity

## Just Finished

Step 7 complete: focused proof now covers structured template argument identity
inside the delegated parser/Sema/HIR checkpoint. Added
`frontend_parser_lookup_authority_tests` probes for type arguments that remain
equal across rendered tag formatting drift when structured `TextId` identity
matches, type arguments with the same rendered tag that stay distinct when
structured `TextId` identity differs, and value arguments with the same stale
rendered `$expr:` text that stay distinct through structured expression-tree
payloads.

## Suggested Next

Next coherent packet: supervisor handoff to plan-owner lifecycle close review
for `ideas/open/149_template_instantiation_structured_argument_key.md`. The
Step 7 proof and coverage indicate the source idea appears ready for close
review, subject to supervisor/reviewer acceptance of the remaining compatibility
mirrors noted below.

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
- Step 7 did not require implementation edits. The new probes are intentionally
  equality/inequality assertions against existing structured key construction,
  not expectation weakening.

## Proof

Step 7 proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_hir_|frontend_parser_lookup_authority_tests|positive_sema_|frontend_cxx_)'`

Result: build succeeded and 145/145 selected tests passed. Proof log:
`test_after.log`.

Coverage rationale: `frontend_parser_lookup_authority_tests` now directly
proves formatting-stable type identity and ambiguous rendered type/value
argument separation; the existing HIR and `cpp_hir_` checkpoint covers the
structured template/HIR consumers after the string-key path was removed or
isolated.

Supervisor regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: pass; 145 passed before and after, with no new failing tests.
