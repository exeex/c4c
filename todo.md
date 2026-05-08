Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Thread Structured Owner Identity To Consumers

# Current Packet

## Just Finished

Completed Step 4 helper slice: struct-definition owner registration/indexing now uses an AST-node owner-key helper that prefers complete `qualifier_text_ids`, canonicalizes parser-owned ids into `Module::link_name_texts` when a link-table spelling exists, and avoids rendered `qualifier_segments` as owner authority when structured ids are complete.

- Added `make_ast_node_ns_qual_for_owner_key` and `make_ast_node_unqualified_text_id_for_owner_key` in HIR lowering helpers.
- Updated `Lowerer::make_struct_def_node_owner_key` and `Lowerer::lower_struct_def` so both node-owner registration and `Module::struct_def_owner_index` use the same canonical owner key path.
- Added active `frontend_hir_lookup_tests` coverage for stale qualifier strings losing to structured TextIds and parser-owned TextId collisions being canonicalized into `link_name_texts`.

## Suggested Next

Next packet: inventory remaining TypeSpec and expression owner-key builders that still copy parser-owned `qualifier_text_ids` directly, then choose one narrow consumer family for the same link-name canonicalization treatment.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- Compound owner meaning still lives in owner segment sequences plus namespace/global qualification.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.
- HIR `try_parse_qualified_struct_method_name` still splits rendered `Node::name`; this slice keeps it only after structured out-of-class metadata is absent or incomplete, not after complete structured misses.
- `make_ns_qual` still uses `qualifier_segments` strings to populate HIR `segment_text_ids`; this packet did not broaden into that shared helper because it has wider declaration/global/type-definition blast radius.
- The helper can only repair parser-owned ids when the link-name table already has or can intern the spelling carrier; consumers without a reliable spelling carrier still need a separate route.
- Keep function/global namespace metadata changes out of the next packet unless the owner-key helper naturally supports them without changing lookup behavior.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_hir|frontend_parser_lookup_authority') > test_after.log 2>&1`

Proof log: `test_after.log`.
