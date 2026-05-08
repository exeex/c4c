Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Thread Structured Owner Identity To Consumers

# Current Packet

## Just Finished

Completed Step 4 member-typedef/template-owner TypeSpec owner-key consumer slice: the local signature `::type` and qualified member-typedef owner-key builders now canonicalize parser-owned qualifier/owner-base `TextId`s through spelling carriers and `Module::link_name_texts` before structured lookup.

- Updated `complete_signature_type_member_lookup_key` to accept the link-name text table and canonicalize individual TypeSpec qualifier/base spelling carriers without rendered owner lookup.
- Updated `complete_qualified_member_typedef_owner_key` to canonicalize each owner qualifier segment and the owner base segment from `qualifier_segments` when available.
- Kept no-spelling fallback structured and non-rendered: it still builds `HirRecordOwnerKey` from available TypeSpec `TextId` metadata and rejects incomplete keys.
- Added focused `frontend_hir_lookup_tests` coverage where direct parser-owned qualifier/base ids would select a wrong collision owner, while the canonicalized owner key resolves the intended member typedef.

## Suggested Next

Next packet: inspect the remaining direct TypeSpec owner-key builders in deferred member-typedef readiness or static/member lookup paths and delegate one narrow spelling-carrier canonicalization slice if it stays inside HIR ownership.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- Compound owner meaning still lives in owner segment sequences plus namespace/global qualification.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.
- HIR `try_parse_qualified_struct_method_name` still splits rendered `Node::name`; this slice keeps it only after structured out-of-class metadata is absent or incomplete, not after complete structured misses.
- `make_ns_qual` still uses `qualifier_segments` strings to populate HIR `segment_text_ids`; this packet did not broaden into that shared helper because it has wider declaration/global/type-definition blast radius.
- The helper can only repair parser-owned ids when the link-name table already has or can intern the spelling carrier; consumers without a reliable spelling carrier still need a separate route.
- The no-module `typespec_aggregate_owner_key(TypeSpec)` path remains a structured parser-id fallback because it has no link-name table for cross-table canonicalization.
- This slice does not canonicalize member typedef `tag_text_id` itself; the touched tests keep member ids canonical and isolate owner-key behavior.
- Keep function/global namespace metadata changes out of the next packet unless the owner-key helper naturally supports them without changing lookup behavior.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_hir|frontend_parser_lookup_authority') > test_after.log 2>&1`

Proof log: `test_after.log`.
