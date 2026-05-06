Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Repair Sema TypeSpec Record Key Width

# Current Packet

## Just Finished

Step 7 final boundary review rejected lifecycle closure.
`review/step7_final_boundary_review.md` found that Sema record completion
helpers rebuild metadata-backed record keys from only `namespace_context_id`
plus `tag_text_id`, dropping the documented
`SemaStructuredNameKey` global-qualification and qualifier-`TextId` fields.

## Suggested Next

Executor should run the new Step 7 repair packet in `plan.md`.
Repair `structured_record_key_for_type()` and
`structured_record_key_for_type_metadata()` so metadata-backed `TypeSpec`
record completion preserves `is_global_qualified` and qualifier `TextId`
sequence, or explicitly implements a documented Sema-owned canonicalization
rule proving those fields are redundant for this path.

## Watchouts

Do not treat namespace context plus base tag `TextId` as enough to ignore
conflicting global or qualifier metadata unless Sema owns and documents that
canonicalization. Do not repair closure by weakening expectations, marking a
supported path unsupported, adding named-case shortcuts, widening parser-map
authority, or recovering identity from rendered strings, `debug_text`,
`@origin:args`, rendered module names, or encoded template-instance names.

Keep this packet narrow to Sema key extraction and direct proof. The source
idea already requires Sema to resolve record tags across namespace context,
qualifier path, record kind, typedef interactions, and incomplete-to-complete
lifecycle, so no durable source-idea edit is currently needed.

## Proof

Expected proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|frontend_hir_tests|cpp_hir_.*structured_metadata)$' > test_after.log`

Add or update focused Sema/frontend coverage where the same namespace context
and base tag `TextId` do not complete through a mismatched global
qualification or qualifier path, unless a documented Sema-owned
canonicalization rule proves that metadata cannot disagree.
